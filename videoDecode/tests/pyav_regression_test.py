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
from pathlib import Path
import tempfile
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


def check_decode(path, pts_offset=0, clear_timestamps=False, separate_planes=True):
    with av.open(str(path)) as container:
        references = [(hashlib.sha256(frame_bytes(f)).digest(),
                       int(f.pts * f.time_base * 1000) if f.pts is not None else 0)
                      for f in container.decode(video=0)]
    assert references, "Reference decoder returned no frames"
    if clear_timestamps:
        references = [(pixels, 0) for pixels, _ in references]
    elif pts_offset:
        references = [(pixels, pts + pts_offset) for pixels, pts in references]
    output = []
    saved = None
    with demuxer(path) as mux:
        cpu = decodercpu(GetRocDecCodecID(mux.GetCodecId()), mem_type=2)
        while True:
            packet = mux.DemuxFrame()
            if not packet.end_of_stream:
                packet.frame_pts += pts_offset
                if clear_timestamps:
                    packet.pkt_flags &= ~int(native.decTypes.ROCDEC_PKT_TIMESTAMP)
            count = cpu.DecodeFrame(packet)
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
                    saved = (buffers, pixels)
                cpu.ReleaseFrame(packet)
            if packet.end_of_stream:
                assert cpu.DecodeFrame(packet) == 0
                break
    del cpu, packet
    gc.collect()
    if output != references:
        print("First mismatches:", [(i, a[0] == b[0], a[1], b[1]) for i, (a, b) in enumerate(zip(output, references)) if a != b][:10])
    assert output == references, f"CPU pixels/timestamps differ for {path}: {len(output)} vs {len(references)}"
    assert b"".join(np.from_dlpack(b).tobytes() for b in saved[0]) == saved[1], "Released frame allocation lost"
    layout = "separate" if separate_planes else "combined"
    print(f"CPU pixel/timestamp/ownership match: {Path(path).name}, {len(output)} frames, {layout}")


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
    codec = GetRocDecCodecID("h264")
    for constructor, latency_option in (
        (decodercpu, "b_force_zero_latency"),
        (native.PyRocVideoDecoderCpu, "force_zero_latency"),
    ):
        constructor(codec=codec, **{latency_option: False, "max_width": 0, "max_height": 0})
        for options, message in (
            ({latency_option: True}, "zero_latency"),
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=Path, required=True)
    args = parser.parse_args()
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
        assert packet_digest(file_mux) == packet_digest(mem_mux)
    with stream_provider(args.input) as provider:
        size = provider.GetBufferSize()
        provider.seek(size + 10)
        assert provider.GetBufferSize() == 0
        assert provider.GetData(bytearray(8), 8) == 0
        provider.seek(size - 3)
        target = bytearray(8)
        assert provider.GetData(target, 8) == 3
        assert target[:3] == args.input.read_bytes()[-3:]
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
    check_decode(args.input)
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
                for index in range(4):
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
                check_decode(path, pts_offset=10000)
                check_decode(path, clear_timestamps=True)
    print("PYAV_REGRESSIONS_PASS")


if __name__ == "__main__":
    main()
