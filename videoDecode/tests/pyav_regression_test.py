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

"""Check PyAV packet ownership, timestamps, seeking and GPU decoding.

This test uses PyAV to decode on the CPU, then compares the result with
rocDecode's GPU-decoded output to verify correctness.

CPU decoding is used only as an independent test reference. The rocPyDecode
CPU decoder APIs and samples have been removed; the library uses PyAV only
for demuxing and rocDecode for GPU decoding.
"""
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
from threading import Event
from types import SimpleNamespace
from unittest.mock import patch
import av
import numpy as np
import rocpydecode as native
from pyRocVideoDecode.demuxer import demuxer, stream_provider
from pyRocVideoDecode.decoder import decoder, GetRocDecCodecID


def check_gpu_decode(path, seek_frame=None):
    # Compare GPU NV12 pixels with an independent software reference. PyAV
    # decoding is used only by this test, never by the rocPyDecode API.
    with demuxer(path) as mux:
        packet = mux.DemuxFrame() if seek_frame is None else mux.SeekFrame(seek_frame, 1, 0)
        start = packet._av_packet.pts * packet._av_packet.time_base
        expected = []
        with av.open(str(path)) as reference:
            for frame in reference.decode(video=0):
                if frame.pts * frame.time_base >= start:
                    pixels = frame.to_ndarray(format="nv12").tobytes()
                    expected.append((int(frame.pts * frame.time_base * 1000),
                                     hashlib.sha256(pixels).digest()))
        gpu = decoder(GetRocDecCodecID(mux.GetCodecId()), mem_type=2)
        actual = []
        while True:
            for _ in range(gpu.DecodeFrame(packet)):
                pts = gpu.GetFrameYuv(packet, separate_planes=True)
                assert pts != -1
                pixels = b"".join(np.from_dlpack(buffer).tobytes()
                                  for buffer in packet.ext_buf if buffer.shape)
                actual.append((pts, hashlib.sha256(pixels).digest()))
                gpu.ReleaseFrame(packet)
            if packet.end_of_stream:
                break
            packet = mux.DemuxFrame()
        assert expected and actual == expected, ("GPU pixels/timestamps differ", path, seek_frame,
                                                 len(actual), len(expected),
                                                 [(i, a[0], e[0], a[1] == e[1])
                                                  for i, (a, e) in enumerate(zip(actual, expected))
                                                  if a != e][:5])
    print("GPU_PIXELS_PASS", path.name, "seek", seek_frame, "frames", len(actual))


def packet_digest(mux):
    result = []
    while True:
        p = mux.DemuxFrame()
        if p.end_of_stream:
            assert p.bitstream_size == 0
            break
        result.append((hashlib.sha256(ctypes.string_at(p.bitstream_adrs, p.bitstream_size)).digest(), p.frame_pts))
    return result


def check_seeking(path, bframes):
    with av.open(str(path)) as container:
        stream = container.streams.video[0]
        rate, time_base, start = stream.average_rate, stream.time_base, stream.start_time or 0
        frames = list(container.decode(video=0))
    assert any(frame.pict_type == 3 for frame in frames) == bool(bframes)
    frame_times = [frame.pts * frame.time_base for frame in frames]
    with demuxer(path) as mux:
        packets = []
        while True:
            packet = mux.DemuxFrame()
            if packet.end_of_stream:
                break
            packets.append((packet._av_packet.pts, packet._av_packet.is_keyframe,
                            hashlib.sha256(ctypes.string_at(packet.bitstream_adrs, packet.bitstream_size)).digest()))
    assert sorted(pts * time_base for pts, _, _ in packets) == frame_times
    # Include backwards seeks, GOP boundaries, the last frame, EOF and a seek after EOF.
    positions = list(range(len(frames))) + [len(frames) + 2, 1, 0]
    for use_provider in (False, True):
        with (stream_provider(path) if use_provider else nullcontext(path)) as source, demuxer(source) as mux:
            for criteria, values in ((0, positions), (1, (0, 1, 2, 10, 0))):
                for value in values:
                    target = Fraction(value, 1) / rate / time_base + start if criteria == 0 else Fraction(value, 1) / time_base + start
                    if criteria == 0:
                        target = int(target)
                    candidates = [(pts, index) for index, (pts, _, _) in enumerate(packets) if pts >= target]
                    actual = mux.SeekFrame(value, 0, criteria)
                    if not candidates:
                        assert actual.end_of_stream and mux.DemuxFrame().end_of_stream
                        continue
                    pts, index = min(candidates)
                    assert actual._av_packet.pts == pts, (path, value, criteria, actual._av_packet.pts, pts)
                    assert actual.frame_pts == int(pts * time_base * 1000)
                    digest = hashlib.sha256(ctypes.string_at(actual.bitstream_adrs, actual.bitstream_size)).digest()
                    assert digest == packets[index][2]
                    # Retain all packets from the first candidate onward, including
                    # larger PTS values encountered before the selected packet.
                    first = min(i for _, i in candidates)
                    assert packet_digest(mux) == [(digest, int(pts * time_base * 1000))
                                                 for i, (pts, _, digest) in enumerate(packets)
                                                 if i >= first and i != index]
            for value in (0, 11, 12, len(frames) - 1, 0):
                target = int(Fraction(value, 1) / rate / time_base) + start
                expected = max(pts for pts, key, _ in packets if key and pts <= target)
                actual = mux.SeekFrame(value, 1, 0)
                assert actual._av_packet.is_keyframe and actual._av_packet.pts == expected
    # Never silently substitute a decoding timestamp for missing presentation metadata.
    with demuxer(path) as mux:
        packet = av.Packet(b"packet")
        packet.dts = 0
        with patch.object(mux, "_next_packet", return_value=packet):
            try:
                mux.SeekFrame(0, 0, 0)
            except ValueError as error:
                assert "presentation timestamps" in str(error)
            else:
                raise AssertionError("Exact seeking accepted missing PTS")
    # Replacing a seek candidate must not lose it or any intervening lookahead.
    # Exercise EOF, an exact match, repeated replacements and equal timestamps,
    # both with DTS bounds and with PTS-only metadata.
    for timestamps in ((4, 2, 3), (4, 3, 2, 5, 6), (4, 0, 1, 3), (4, 2, 2, 3)):
        for with_dts in (False, True):
            with demuxer(path) as mux:
                packets = []
                for index, value in enumerate(timestamps):
                    packet = av.Packet(bytes([index]))
                    packet.pts = start + int(Fraction(value, 1) / time_base)
                    packet.time_base = time_base
                    if with_dts:
                        packet.dts = start + int(Fraction(index - 1, 1) / time_base)
                    packets.append(packet)
                # Keep the real queue logic, substituting only the container input.
                source = SimpleNamespace(seek=mux._container.seek, demux=lambda stream: (p for p in packets))
                with patch.object(mux, "_container", source), patch.object(mux, "_filter", None):
                    actual = mux.SeekFrame(1, 0, 1)
                    selected = min((p for p in packets if p.pts >= start + 1 / time_base),
                                   key=lambda p: p.pts)
                    assert actual._av_packet is selected
                    remaining = []
                    while not (packet := mux.DemuxFrame()).end_of_stream:
                        remaining.append(packet._av_packet)
                    assert remaining == [p for p in packets if p is not selected]
                    assert mux.DemuxFrame().end_of_stream
    print("SEEK_PASS", path.name)


def check_seek_variants(directory):
    for extension in ("mp4", "mkv"):
        for rate, offset, bframes in ((Fraction(24), 0, 3), (Fraction(30000, 1001), 150, 3),
                                     (Fraction(24), 48, 0)):
            path = Path(directory) / f"seek-{rate.numerator}-{offset}-{bframes}.{extension}"
            with av.open(str(path), "w") as container:
                stream = container.add_stream("libx264", rate=rate)
                stream.width, stream.height, stream.pix_fmt = 64, 48, "yuv420p"
                stream.options = {"crf": "20", "bf": str(bframes), "b-adapt": "0", "g": "12", "sc_threshold": "0"}
                for index in range(48):
                    frame = av.VideoFrame(64, 48, "yuv420p")
                    frame.pts = offset + index
                    for plane, value in zip(frame.planes, (32 + index * 3, 128, 128)):
                        plane.update(bytes([value]) * plane.buffer_size)
                    for packet in stream.encode(frame):
                        container.mux(packet)
                for packet in stream.encode(None):
                    container.mux(packet)
            check_seeking(path, bframes)


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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=Path, required=True)
    args = parser.parse_args()
    check_empty_buffers()
    check_provider_storage()
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
        closed = Event()
        def read_until_closed():
            for _ in range(20):
                try:
                    mux.DemuxFrame()
                except ValueError as error:
                    assert "closed" in str(error)
                    return
            # The reader may finish its bounded loop before the closer runs.
            # It must still observe a rejected read after close completes.
            assert closed.wait(timeout=10), "Demuxer close did not complete"
            try:
                mux.DemuxFrame()
            except ValueError as error:
                assert "closed" in str(error)
            else:
                raise AssertionError("Concurrent reader accepted a read after close")
        def close():
            try:
                mux.close()
            finally:
                closed.set()
        with ThreadPoolExecutor(max_workers=2) as workers:
            reading = workers.submit(read_until_closed)
            closing = workers.submit(close)
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
    # PyAV supplies test references only; rocPyDecode decodes on the GPU.
    check_gpu_decode(args.input)
    check_gpu_decode(args.input, seek_frame=300)
    with tempfile.TemporaryDirectory() as tmp:
        check_seek_variants(tmp)
    print("PYAV_REGRESSIONS_PASS")


if __name__ == "__main__":
    main()
