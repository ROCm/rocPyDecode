# Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""Check PyAV packet ownership, CPU pixels, timestamps and decoder draining."""
import argparse
import ctypes
import gc
import hashlib
from concurrent.futures import ThreadPoolExecutor
from contextlib import nullcontext
from pathlib import Path
import tempfile
import tracemalloc
from fractions import Fraction
from types import SimpleNamespace
from unittest.mock import patch
import av
import numpy as np
import rocpydecode as native
from pyRocVideoDecode.demuxer import demuxer, stream_provider
from pyRocVideoDecode.decodercpu import decodercpu
from pyRocVideoDecode.decoder import GetRocDecCodecID


def frame_bytes(frame):
    itemsize = 2 if max(c.bits for c in frame.format.components) > 8 else 1
    parts = []
    for plane in frame.planes:
        data = memoryview(plane)
        parts.extend(data[y * plane.line_size:y * plane.line_size + plane.width * itemsize]
                     for y in range(plane.height))
    return b"".join(parts)


def check_decode(path, pts_offset=0, clear_timestamps=False, separate_planes=True, payload_eos=False, use_provider=False):
    references = []
    reference_depth = None
    with av.open(str(path)) as container:
        for frame in container.decode(video=0):
            if reference_depth is None:
                reference_depth = max(component.bits for component in frame.format.components)
            references.append((hashlib.sha256(frame_bytes(frame)).digest(),
                               int(frame.pts * frame.time_base * 1000) if frame.pts is not None else 0))
    assert references, "Reference decoder returned no frames"
    if clear_timestamps:
        references = [(pixels, 0) for pixels, _ in references]
    elif pts_offset:
        references = [(pixels, pts + pts_offset) for pixels, pts in references]
    output = []
    saved = None
    returned = 0
    with (stream_provider(path) if use_provider else nullcontext(path)) as source, demuxer(source) as mux:
        # Metadata queries before decoding must preserve the packet position.
        assert mux.GetBitDepth() == reference_depth
        assert mux.GetBitDepth() == reference_depth
        cpu = decodercpu(GetRocDecCodecID(mux.GetCodecId()))
        packet = mux.DemuxFrame()
        while True:
            following = mux.DemuxFrame() if payload_eos and packet.bitstream_size else None
            if following is not None and following.end_of_stream:
                packet.pkt_flags |= int(native.decTypes.ROCDEC_PKT_ENDOFSTREAM)
            if not packet.end_of_stream:
                packet.frame_pts += pts_offset
                if clear_timestamps:
                    packet.pkt_flags &= ~int(native.decTypes.ROCDEC_PKT_TIMESTAMP)
            count = cpu.DecodeFrame(packet)
            returned += count
            if count:
                assert cpu.GetStride() > 0 and cpu.GetFrameSize() > 0
                assert cpu.GetOutputSurfaceInfo(), "Metadata must be available before retrieving a frame"
            for _ in range(count):
                pts = cpu.GetFrameYuv(packet, separate_planes=separate_planes)
                assert pts != -1
                parts = []
                buffers = packet.ext_buf[:] if separate_planes else packet.ext_buf[:1]
                for buf in buffers:
                    rows, width = buf.shape
                    size = width * (2 if buf.dtype == "|u2" else 1)
                    # Separate planes are packed by the native surface helper.
                    view = np.from_dlpack(buf)
                    parts.append(view.tobytes())
                    assert len(parts[-1]) == rows * size
                    assert buf.__dlpack_device__()[0] == 1
                    if not separate_planes:
                        assert width == cpu.GetWidth()
                        assert buf.strides == (width, 1)
                        assert view.nbytes == packet.frame_size == cpu.GetFrameSize()
                pixels = b"".join(parts)
                output.append((hashlib.sha256(pixels).digest(), pts))
                if saved is None:
                    saved = (buffers, pixels, tuple(np.from_dlpack(b) for b in buffers))
                cpu.ReleaseFrame(packet)
                assert all(not b.shape for b in packet.ext_buf), "Released packet retains frame exports"
            if packet.pkt_flags & int(native.decTypes.ROCDEC_PKT_ENDOFSTREAM):
                assert returned + cpu.GetNumOfFlushedFrames() == len(references), "Drained frames counted twice"
                if packet.bitstream_size:
                    try:
                        cpu.DecodeFrame(packet)
                    except ValueError as error:
                        assert "end of stream" in str(error)
                    else:
                        raise AssertionError("CPU decoder accepted a payload after EOS")
                assert cpu.DecodeFrame(native.PyPacketData()) == 0
                assert cpu.GetNumOfFlushedFrames() == 0
                break
            packet = following if following is not None else mux.DemuxFrame()
    del cpu, packet
    gc.collect()
    if output != references:
        print("First mismatches:", [(i, a[0] == b[0], a[1], b[1]) for i, (a, b) in enumerate(zip(output, references)) if a != b][:10])
    assert output == references, f"CPU pixels/timestamps differ for {path}: {len(output)} vs {len(references)}"
    assert b"".join(np.from_dlpack(b).tobytes() for b in saved[0]) == saved[1], "Released frame allocation lost"
    assert b"".join(view.tobytes() for view in saved[2]) == saved[1], "Retained DLPack views lost their allocation"
    layout = "separate" if separate_planes else "combined"
    print(f"CPU pixel/timestamp/ownership match: {Path(path).name}, {len(output)} frames, {layout}")


def check_cpu_metadata_changes():
    frames = []
    for width, height, fmt in ((32, 24, "yuv420p"), (48, 32, "yuv420p"),
                               (48, 32, "yuv422p10le"), (32, 24, "yuv444p")):
        encoder = av.CodecContext.create("libx264", "w")
        encoder.width, encoder.height, encoder.pix_fmt = width, height, fmt
        encoder.time_base = Fraction(1, 24)
        encoder.options = {"crf": "0", "preset": "ultrafast", "tune": "zerolatency"}
        frame = av.VideoFrame(width, height, fmt)
        for index, plane in enumerate(frame.planes):
            plane.update(bytes([17 + index]) * plane.buffer_size)
        frames.extend(encoder.encode(frame) + encoder.encode(None))
    reference = av.CodecContext.create("h264", "r")
    expected = []
    cpu = decodercpu("h264")
    assert (cpu.GetWidth(), cpu.GetHeight(), cpu.GetStride(), cpu.GetFrameSize(), cpu.GetBitDepth()) == (0, 0, 0, 0, 0)
    for encoded in frames:
        expected.extend(reference.decode(encoded))
        packet = native.GetRocPyDecPacket(0, encoded.size, bytes(encoded))
        cpu.DecodeFrame(packet)
    expected.extend(reference.decode(None))
    cpu.DecodeFrame(native.PyPacketData())
    assert len(expected) == len(cpu._frames) == 4
    retained = []
    for frame in expected:
        depth = max(c.bits for c in frame.format.components)
        assert cpu.GetWidth() == frame.width, "Queued frame width is stale"
        assert cpu.GetHeight() == frame.height, "Queued frame height is stale"
        assert cpu.GetStride() == frame.width * (2 if depth > 8 else 1)
        assert cpu.GetFrameSize() == len(frame_bytes(frame))
        assert cpu.GetBitDepth() == depth, "Queued frame bit depth is stale"
        info = cpu.GetOutputSurfaceInfo()
        assert tuple((ctypes.c_uint32 * 3).from_address(info)) == (frame.width, frame.height, cpu.GetStride())
        surface_frame = cpu._surface_frame
        assert cpu.GetOutputSurfaceInfo() and cpu._surface_frame is surface_frame
        packet = native.PyPacketData()
        assert cpu.GetFrameYuv(packet, separate_planes=True) != -1
        cpu.ReleaseFrame(native.PyPacketData())  # Releasing another packet must not change this frame.
        # Queries used by resize/save must still describe the retrieved frame.
        assert cpu.GetWidth() == frame.width and cpu.GetBitDepth() == depth
        assert cpu.ResizeFrame(packet, (frame.width, frame.height), cpu.GetOutputSurfaceInfo()) == 0
        views = [np.from_dlpack(b) for b in packet.ext_buf if b.shape]
        pixels = frame_bytes(frame)
        assert b"".join(v.tobytes() for v in views) == pixels
        retained.append((views, pixels))
        with tempfile.TemporaryDirectory() as tmp:
            saved = Path(tmp) / "frame.yuv"
            cpu.SaveFrameToFile(str(saved), packet.frame_adrs)
            assert saved.read_bytes() == pixels
        cpu.ReleaseFrame(packet)
    assert cpu.GetWidth() == expected[-1].width and cpu.GetBitDepth() == 8
    assert cpu.GetFrameYuv(native.PyPacketData()) == -1
    for views, pixels in retained:
        assert b"".join(v.tobytes() for v in views) == pixels
    print("CPU queued resolution/format/depth metadata and pixels match")


def check_av1_capabilities():
    for available, selected in (({"libdav1d"}, "libdav1d"),
                                ({"libaom-av1"}, "libaom-av1"),
                                ({"av1", "libdav1d", "libaom-av1"}, "libdav1d"),
                                ({"av1"}, "av1"), (set(), None)):
        calls = []
        def codec(name, mode):
            calls.append(name)
            if name not in available and name != "h264":
                raise ValueError("Decoder unavailable")
            return SimpleNamespace(name=name)
        backend = SimpleNamespace(codecs_available=available, Codec=codec,
                                  CodecContext=SimpleNamespace(create=codec))
        with patch("pyRocVideoDecode.decodercpu.require_av", return_value=backend):
            probe = decodercpu("h264")
            for value in ("av1", 225, native.decTypes.rocDecVideoCodec_AV1):
                assert probe.IsCodecSupported(0, value, 8) == (selected is not None)
                if selected is not None:
                    decoder = decodercpu(value)
                    assert decoder._codec.name == selected and calls[-2:] == [selected, selected]
                else:
                    try:
                        decodercpu(value)
                    except ValueError:
                        pass
                    else:
                        raise AssertionError("Constructed an unavailable decoder")
            assert not probe.IsCodecSupported(0, "av1", 9)
            assert not probe.IsCodecSupported(0, "unsupported", 8)
    print("AV1 capability checks match decoder selection")


def check_rgb_reuse(path):
    hip = ctypes.CDLL("libamdhip64.so")
    hip.hipMemcpy.argtypes = (ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int)
    hip.hipMemcpy.restype = ctypes.c_int

    def read_pixels(address, size):
        data = ctypes.create_string_buffer(size)
        assert hip.hipMemcpy(data, address, size, 2) == 0  # Device to host.
        return data.raw

    for memory in (1, 2):
        retained = []
        addresses = []
        with demuxer(path) as mux:
            cpu = decodercpu(GetRocDecCodecID(mux.GetCodecId()), mem_type=memory)
            available = 0
            for index, fmt in enumerate((5, 5, 5, 5, 5, 5, 1, 2, 5, 5)):
                while available == 0:
                    packet = mux.DemuxFrame()
                    available = cpu.DecodeFrame(packet)
                    assert available or not packet.end_of_stream, "Not enough RGB test frames"
                assert cpu.GetFrameRgb(packet, fmt) != -1
                available -= 1
                address = packet.frame_adrs_rgb
                size = cpu.GetWidth() * cpu.GetHeight() * (4 if fmt >= 5 else 3) * (2 if fmt % 2 == 0 else 1)
                if index in (1, 3, 5, 9):
                    assert address == addresses[-1], "Released RGB allocation was not reused"
                elif index:
                    assert address != addresses[-1], "Retained or differently sized RGB allocation was reused"
                addresses.append(address)
                if index in (1, 3):
                    # Retain one buffer and one DLPack capsule independently of the packet.
                    owner = packet.ext_buf[0] if index == 1 else packet.ext_buf[0].__dlpack__()
                    retained.append((owner, address, read_pixels(address, size)))
                cpu.ReleaseFrame(packet)
                assert all(not b.shape for b in packet.ext_buf)
        del cpu, packet
        gc.collect()
        for owner, address, pixels in retained:
            assert read_pixels(address, len(pixels)) == pixels, "Retained RGB pixels were overwritten"
    print("CPU RGB reuse, format changes and retained buffer/DLPack pixels passed")


def packet_digest(mux):
    result = []
    while True:
        p = mux.DemuxFrame()
        if p.end_of_stream:
            assert p.bitstream_size == 0
            break
        result.append((hashlib.sha256(ctypes.string_at(p.bitstream_adrs, p.bitstream_size)).digest(), p.frame_pts))
    return result


def check_cpu_options():
    from pyRocVideoDecode import PyVideoDemuxer, PyFileStreamProvider, PyRocVideoDecoderCpu
    from pyRocVideoDecode.decodercpu import PyRocVideoDecoderCpu as CpuClass
    assert PyVideoDemuxer is demuxer
    assert PyFileStreamProvider is stream_provider
    assert PyRocVideoDecoderCpu is CpuClass
    codec = GetRocDecCodecID("h264")
    for constructor in (decodercpu, native.PyRocVideoDecoderCpu):
        constructor(codec=codec, max_width=0, max_height=0)
        for options, message in (
            ({"max_width": 1920}, "max_width"),
            ({"max_height": 1080}, "max_height"),
            ({"max_width": 1920, "max_height": 1080}, "max_width"),
        ):
            try:
                constructor(codec=codec, **options)
            except ValueError as error:
                assert message in str(error), str(error)
            else:
                raise AssertionError(f"Unsupported CPU options accepted: {options}")


def check_provider_storage():
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "input.bin"
        size = 8 * 1024 * 1024
        with path.open("wb") as output:
            output.write(b"head")
            output.seek(size - 4)
            output.write(b"tail")
        target = bytearray(2 * 1024 * 1024)
        tracemalloc.start()
        try:
            with stream_provider(path) as provider:
                assert tracemalloc.get_traced_memory()[1] < 1024 * 1024, "Provider loaded the entire file"
                assert provider.GetFileStreamProvider() is provider
                assert provider.readable() and provider.seekable() and not provider.writable()
                assert provider.GetBufferSize() == size
                tracemalloc.reset_peak()
                baseline = tracemalloc.get_traced_memory()[0]
                assert provider.GetData(target, len(target)) == len(target)
                assert tracemalloc.get_traced_memory()[1] - baseline < 1024 * 1024, "GetData copied the read buffer"
                assert target[:4] == b"head" and not any(memoryview(target)[4:])
                assert provider.GetBufferSize() == size - len(target)
                provider.seek(-4, 2)
                assert provider.read(4) == b"tail" and provider.GetBufferSize() == 0
                provider.seek(0)
                try:
                    provider.GetData(memoryview(target)[::2], 4)
                except TypeError:
                    assert provider.tell() == 0
                else:
                    raise AssertionError("Noncontiguous destination accepted")
                assert provider.GetData(bytearray(), 4) == 0 and provider.tell() == 0
        finally:
            tracemalloc.stop()
        assert provider.closed
        provider.close()
        for operation in (provider.GetBufferSize, lambda: provider.read(1),
                          lambda: provider.GetData(target, 1), lambda: provider.GetData(target, 0),
                          lambda: provider.GetData(target, -1),
                          lambda: provider.GetData(bytearray(), 1), lambda: provider.seek(0)):
            try:
                operation()
            except ValueError:
                pass
            else:
                raise AssertionError("Closed provider accepted I/O")
        path.write_bytes(b"")
        with stream_provider(path) as empty:
            assert empty.GetBufferSize() == 0 and empty.GetData(target, 4) == 0
        try:
            stream_provider(path.with_name("missing"))
        except FileNotFoundError:
            pass
        else:
            raise AssertionError("Missing file accepted")
    print("File-backed provider memory and I/O checks passed")


def check_empty_buffers():
    packet = native.PyPacketData()
    for obj in (packet, *packet.ext_buf):
        for operation in (obj.__dlpack__, obj.__dlpack_device__):
            try:
                operation()
            except RuntimeError as error:
                assert "uninitialized buffer" in str(error)
            else:
                raise AssertionError("Uninitialized buffer accepted DLPack access")
    for buffers in ([], [None], [None, None, None]):
        packet.ext_buf = buffers
        for name in ("shape", "shapeY", "shapeU", "shapeUV", "shapeV", "strides", "dtype",
                     "__dlpack__", "__dlpack_device__"):
            try:
                value = getattr(packet, name)
                if callable(value):
                    value()
            except RuntimeError as error:
                assert "no buffer" in str(error)
            else:
                raise AssertionError(f"Missing packet buffer accepted {name}")
    print("Empty and missing packet buffers raise Python errors")


def check_full_range_rgb():
    hip = ctypes.CDLL("libamdhip64.so")
    hip.hipMemcpy.argtypes = (ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int)
    hip.hipMemcpy.restype = ctypes.c_int
    # Include real JPEG encode/decode, all chroma layouts and tagged non-JPEG
    # full-range frames. Constant chroma avoids upsampling differences in references.
    cases = [(fmt, 8, 2) for fmt in ("yuvj420p", "yuvj422p", "yuvj444p")]
    cases += [(fmt, depth, matrix) for fmt, depth in
              (("yuv420p", 8), ("yuv422p10le", 10), ("yuv444p16le", 16))
              for matrix in (1, 6, 9)]
    for fmt, depth, matrix in cases:
        frame = av.VideoFrame(34, 26, fmt)
        frame.color_range = 2
        frame.colorspace = matrix
        dtype = np.uint8 if depth == 8 else np.dtype("<u2")
        shift = depth - 8
        for index, plane in enumerate(frame.planes):
            data = np.zeros(plane.buffer_size // np.dtype(dtype).itemsize, dtype=dtype)
            rows = data.reshape(plane.height, plane.line_size // np.dtype(dtype).itemsize)
            rows[:, :plane.width] = ((np.arange(plane.width) * 255 // (plane.width - 1)) << shift
                                     if index == 0 else (104 if index == 1 else 152) << shift)
            plane.update(data.tobytes())
        if fmt.startswith("yuvj"):
            encoder = av.CodecContext.create("mjpeg", "w")
            encoder.width, encoder.height, encoder.pix_fmt = frame.width, frame.height, fmt
            encoder.time_base = Fraction(1, 24)
            encoded = encoder.encode(frame) + encoder.encode(None)
            reference = av.CodecContext.create("mjpeg", "r")
            decoded = [f for packet in encoded for f in reference.decode(packet)]
            frame = decoded[0]
        # Independent floating-point full-range YCbCr equations on decoded samples.
        planes = [np.ndarray((p.height, p.width), dtype=dtype, buffer=memoryview(p),
                             strides=(p.line_size, np.dtype(dtype).itemsize)).astype(np.float64)
                  for p in frame.planes]
        y, u, v = planes
        u = np.repeat(np.repeat(u, y.shape[0] // u.shape[0], 0), y.shape[1] // u.shape[1], 1)
        v = np.repeat(np.repeat(v, y.shape[0] // v.shape[0], 0), y.shape[1] // v.shape[1], 1)
        u -= 1 << (depth - 1)
        v -= 1 << (depth - 1)
        kr, kb = {1: (0.2126, 0.0722), 9: (0.2627, 0.0593)}.get(matrix, (0.299, 0.114))
        expected = np.stack((y + 2 * (1 - kr) * v,
                             y - 2 * kb * (1 - kb) / (1 - kr - kb) * u
                               - 2 * kr * (1 - kr) / (1 - kr - kb) * v,
                             y + 2 * (1 - kb) * u), axis=-1)
        expected = np.clip(expected, 0, (1 << depth) - 1).astype(np.uint32)
        for memory in (1, 2):
            cpu = decodercpu("mjpeg", mem_type=memory, crop_rect=(2, 2, 32, 24))
            if memory == 2:
                yuv = decodercpu("mjpeg")
                yuv._frames.append(frame)
                packet = native.PyPacketData()
                assert yuv.GetFrameYuv(packet, separate_planes=True) != -1
                assert b"".join(np.from_dlpack(b).tobytes() for b in packet.ext_buf) == frame_bytes(frame)
                yuv.ReleaseFrame(packet)
            for output_format in range(1, 9):
                if fmt.startswith("yuvj"):
                    cpu = decodercpu("mjpeg", mem_type=memory, crop_rect=(2, 2, 32, 24))
                    for packet in encoded:
                        cpu.DecodeFrame(native.GetRocPyDecPacket(0, packet.size, bytes(packet)))
                    cpu.DecodeFrame(native.PyPacketData())
                else:
                    cpu._frames.append(frame)
                packet = native.PyPacketData()
                assert cpu.GetFrameRgb(packet, output_format) != -1
                channels = 4 if output_format >= 5 else 3
                bits = 16 if output_format % 2 == 0 else 8
                actual = np.empty((22, 30, channels), dtype=np.uint16 if bits == 16 else np.uint8)
                assert hip.hipMemcpy(actual.ctypes.data, packet.frame_adrs_rgb, actual.nbytes, 2) == 0
                target = expected[2:24, 2:32].copy()
                target = target << (bits - depth) if bits >= depth else target >> (depth - bits)
                if output_format in (1, 2, 5, 6):
                    target = target[:, :, ::-1]
                tolerance = 1 << max(bits - depth, 0)
                np.testing.assert_allclose(actual[:, :, :3], target, rtol=0, atol=tolerance)
                if fmt.startswith("yuvj") and output_format == 3:
                    reference_rgb = frame.to_ndarray(format="rgb24")[2:24, 2:32]
                    np.testing.assert_allclose(actual, reference_rgb, rtol=0, atol=2)
                if channels == 4:
                    assert not actual[:, :, 3].any()
                assert packet.__dlpack_device__() == (10, 0)
                cpu.ReleaseFrame(packet)
                try:
                    packet.__dlpack_device__()
                except RuntimeError:
                    pass
                else:
                    raise AssertionError("Released packet still reports a tensor device")
    print("Full-range RGB pixels: JPEG and tagged 8/10/16-bit frames, formats 1–8, host/device and crop passed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=Path, required=True)
    args = parser.parse_args()
    check_empty_buffers()
    check_full_range_rgb()
    check_provider_storage()
    check_cpu_options()
    with demuxer(args.input) as mux:
        first = mux.DemuxFrame()
        expected = ctypes.string_at(first.bitstream_adrs, first.bitstream_size)
        mux.DemuxFrame()
    gc.collect()
    assert ctypes.string_at(first.bitstream_adrs, first.bitstream_size) == expected
    mux.close()  # Closing twice is harmless; every read/seek after close fails.
    for operation in (mux.DemuxFrame, mux.GetCodecId, mux.GetBitDepth,
                      lambda: mux.SeekFrame(0, 1, 0), mux.__enter__):
        try:
            operation()
        except ValueError as error:
            assert "closed" in str(error)
        else:
            raise AssertionError("Closed demuxer accepted an operation")
    # Reading and closing from different threads must never free an active input.
    for _ in range(5):
        mux = demuxer(args.input)
        mux.DemuxFrame()
        def read_until_closed():
            for _ in range(20):
                try:
                    mux.DemuxFrame()
                except ValueError as error:
                    assert "closed" in str(error)
                    return
        with ThreadPoolExecutor(max_workers=2) as workers:
            reading = workers.submit(read_until_closed)
            closing = workers.submit(mux.close)
            reading.result(timeout=30)
            closing.result(timeout=30)
    with demuxer(args.input) as file_mux, stream_provider(args.input) as provider, demuxer(provider) as mem_mux:
        assert file_mux.GetBitDepth() == mem_mux.GetBitDepth() > 0
        assert packet_digest(file_mux) == packet_digest(mem_mux)
        for frame in (0, 1):
            direct = file_mux.SeekFrame(frame, 1, 0)
            streamed = mem_mux.SeekFrame(frame, 1, 0)
            assert direct.frame_pts == streamed.frame_pts
            assert ctypes.string_at(direct.bitstream_adrs, direct.bitstream_size) == ctypes.string_at(streamed.bitstream_adrs, streamed.bitstream_size)
        mem_mux.close()
        assert not provider.closed, "Demuxer closed its caller-owned input"
        provider.seek(0)
        with args.input.open("rb") as reference:
            assert provider.read(4) == reference.read(4)
    with stream_provider(args.input) as provider:
        size = provider.GetBufferSize()
        provider.seek(size + 10)
        assert provider.GetBufferSize() == 0
        assert provider.GetData(bytearray(8), 8) == 0
        provider.seek(size - 3)
        target = bytearray(8)
        assert provider.GetData(target, 8) == 3
        with args.input.open("rb") as reference:
            reference.seek(-3, 2)
            assert target[:3] == reference.read(3)
        provider.seek(0)
        for requested in (0, -1):
            assert provider.GetData(target, requested) == 0 and provider.tell() == 0
        try:
            provider.GetData(b"readonly", 3)
        except TypeError:
            assert provider.tell() == 0, "Rejected reads must not consume input"
        else:
            raise AssertionError("Read-only destination accepted")
    with demuxer(args.input) as mux:
        start = mux.DemuxFrame()
        seek = mux.SeekFrame(0, 1, 0)
        assert ctypes.string_at(start.bitstream_adrs, start.bitstream_size) == ctypes.string_at(seek.bitstream_adrs, seek.bitstream_size)
        assert not mux.SeekFrame(0, 0, 1).end_of_stream
        for values in [(-1, 0, 0), (0, 2, 0), (0, 0, 2)]:
            try:
                mux.SeekFrame(*values)
            except ValueError:
                pass
            else:
                raise AssertionError("Invalid seek accepted")
    # Public packet construction must own a contiguous buffer and preserve int64 PTS.
    data = bytearray(b"packet")
    p = native.GetRocPyDecPacket(2**40, len(data), data)
    try:
        data.extend(b"resize")
    except BufferError:
        pass
    else:
        raise AssertionError("Packet did not pin its resizable buffer")
    del data
    gc.collect()
    assert p.frame_pts == 2**40 and ctypes.string_at(p.bitstream_adrs, p.bitstream_size) == b"packet"
    for size, buffer in [(-1, b"a"), (2, b"a"), (2, memoryview(b"abcd")[::2])]:
        try:
            native.GetRocPyDecPacket(0, size, buffer)
        except ValueError:
            pass
        else:
            raise AssertionError("Invalid packet buffer accepted")
    check_cpu_metadata_changes()
    check_av1_capabilities()
    check_decode(args.input)
    check_decode(args.input, use_provider=True)
    for name in ("AMD_driving_virtual_20-H265.mp4", "AMD_driving_virtual_20-AV1.mp4",
                 "AMD_driving_virtual_20-VP9.ivf"):
        sibling = args.input.parent / name
        if sibling.exists() and sibling != args.input:
            check_decode(sibling)
    # Use PyAV itself to prepare tiny compressed fixtures.
    with tempfile.TemporaryDirectory() as tmp:
        for fmt in ("yuv420p", "yuv422p", "yuv444p", "yuv420p10le", "yuv422p10le", "yuv444p10le"):
            path = Path(tmp) / (fmt + ".mkv")
            with av.open(str(path), "w") as container:
                stream = container.add_stream("libx264", rate=24)
                stream.width, stream.height, stream.pix_fmt = 66, 50, fmt
                stream.options = {"crf": "0"}
                if fmt == "yuv420p":
                    stream.options = {"crf": "20", "bf": "2", "b-adapt": "0"}
                for index in range(12 if fmt == "yuv420p" else 4):
                    frame = av.VideoFrame(66, 50, fmt)
                    itemsize = 2 if "10" in fmt else 1
                    for number, plane in enumerate(frame.planes):
                        data = bytearray(plane.buffer_size)
                        rows = np.ndarray((plane.height, plane.line_size // itemsize),
                                          dtype="<u2" if itemsize == 2 else "u1", buffer=data)
                        # Vary every row and column; source row padding stays zero.
                        values = np.arange(plane.height * plane.width, dtype=np.uint32)
                        values = (values * 13 + number * 53 + index * 17) % (1024 if itemsize == 2 else 256)
                        rows[:, :plane.width] = values.reshape(plane.height, plane.width)
                        plane.update(data)
                    frame.pts = index
                    for packet in stream.encode(frame):
                        container.mux(packet)
                for packet in stream.encode(None):
                    container.mux(packet)
            check_decode(path)
            check_decode(path, separate_planes=False)
            if fmt == "yuv420p":
                with av.open(str(path)) as container:
                    assert any(frame.pict_type == 3 for frame in container.decode(video=0)), "EOS fixture needs B-frames"
                check_decode(path, payload_eos=True)
                check_rgb_reuse(path)
                check_decode(path, pts_offset=10000)
                check_decode(path, clear_timestamps=True)
    print("PYAV_REGRESSIONS_PASS")


if __name__ == "__main__":
    main()
