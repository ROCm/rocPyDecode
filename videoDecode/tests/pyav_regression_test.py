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

"""Verify GPU pixels, seeking, packet lifetime and end-of-stream handling.

This test uses PyAV to decode on the CPU, then compares the result with
rocDecode's GPU-decoded output to verify correctness. CPU decoding is only
a test reference; the library uses PyAV for demuxing.
"""
import argparse
import ctypes
from fractions import Fraction
import hashlib
from pathlib import Path
import sys
import tempfile
from contextlib import nullcontext
import rocpydecode as native
from pyRocVideoDecode._pyav import require_av
from pyRocVideoDecode.demuxer import demuxer, stream_provider
from pyRocVideoDecode.decoder import decoder, GetRocDecCodecID

try:
    av = require_av()
    import numpy as np
except ImportError as error:
    print(f"Skipping pyav_regression_test: {error}")
    sys.exit(77)


def check_gpu_decode(path, timestamp=None):
    with demuxer(path) as video_demuxer:
        packet = (video_demuxer.DemuxFrame() if timestamp is None else
                  video_demuxer.SeekFrame(timestamp, 1, 1))
        assert not packet.end_of_stream
        start = packet._av_packet.pts * packet._av_packet.time_base
        expected = []
        with av.open(str(path)) as reference:
            for frame in reference.decode(video=0):
                if frame.pts * frame.time_base >= start:
                    pixels = frame.to_ndarray(format="nv12").tobytes()
                    expected.append((int(frame.pts * frame.time_base * 1000),
                                     hashlib.sha256(pixels).digest()))
        video_decoder = decoder(GetRocDecCodecID(video_demuxer.GetCodecId()),
                                mem_type=native.decTypes.OUT_SURFACE_MEM_HOST_COPIED)
        actual = []
        while True:
            if packet.end_of_stream:
                eos = native.GetRocPyDecPacket(0, 0, b"")
                assert eos.end_of_stream and eos.bitstream_adrs == 0
                assert eos.pkt_flags == packet.pkt_flags == int(native.decTypes.ROCDEC_PKT_ENDOFSTREAM)
                packet = eos
            for _ in range(video_decoder.DecodeFrame(packet)):
                pts = video_decoder.GetFrameYuv(packet, separate_planes=True)
                assert pts != -1
                pixels = b"".join(np.from_dlpack(buffer).tobytes()
                                  for buffer in packet.ext_buf if buffer.shape)
                actual.append((pts, hashlib.sha256(pixels).digest()))
                video_decoder.ReleaseFrame(packet)
            if packet.end_of_stream:
                break
            packet = video_demuxer.DemuxFrame()
        assert expected and actual == expected, ("GPU pixels/timestamps differ", path, timestamp,
                                                 len(actual), len(expected))
    print("GPU_PIXELS_PASS", path.name, "seek", timestamp, "frames", len(actual))


def packet_data(packet):
    return (packet._av_packet.pts,
            hashlib.sha256(ctypes.string_at(packet.bitstream_adrs, packet.bitstream_size)).digest())


def check_seeking(path):
    with demuxer(path) as video_demuxer:
        stream = video_demuxer._stream
        rate, time_base, start = stream.average_rate, stream.time_base, stream.start_time or 0
        packets, keyframes = [], []
        while not (packet := video_demuxer.DemuxFrame()).end_of_stream:
            packets.append(packet_data(packet))
            if packet._av_packet.is_keyframe:
                keyframes.append(packet._av_packet.pts)
    assert packets and keyframes
    for use_provider in (False, True):
        with (stream_provider(path) if use_provider else nullcontext(path)) as source:
            with demuxer(source) as video_demuxer:
                video_demuxer.DemuxFrame()
                previous_packets = video_demuxer._packets
                video_demuxer.SeekFrame(0, 1, 0)
                assert next(previous_packets, None) is None, "Seek left the previous demux iterator open"
                for criteria, values in ((0, (0, 1, 2, 11, 12, len(packets) - 1, len(packets) + 2, 0)),
                                         (1, (0, 0.1, Fraction(1, 3), 1.25, 2.5, 10, 0))):
                    for value in values:
                        seconds = Fraction(value) / rate if criteria == 0 else Fraction(value)
                        target = seconds / time_base + start
                        if criteria == 0:
                            target = int(target)
                        # Exact packet seeking must retain decode order, including B-frames.
                        first = next((i for i, (pts, _) in enumerate(packets) if pts >= target), len(packets))
                        packet = video_demuxer.SeekFrame(value, 0, criteria)
                        if first == len(packets):
                            assert packet.end_of_stream, "Seek past the end must return EOS"
                            assert packet.bitstream_size == 0 and packet.bitstream_adrs == 0
                        actual = []
                        while not packet.end_of_stream:
                            actual.append(packet_data(packet))
                            packet = video_demuxer.DemuxFrame()
                        assert actual == packets[first:], (path, value, criteria)
                        assert packet.pkt_flags == int(native.decTypes.ROCDEC_PKT_ENDOFSTREAM)
                        packet = video_demuxer.SeekFrame(value, 1, criteria)
                        assert packet._av_packet.is_keyframe
                        assert packet._av_packet.pts == max(pts for pts in keyframes if pts <= int(target))
                for value, mode, criteria in ((-1, 0, 0), (0, 2, 0), (0, 0, 2),
                                              (2.5, 0, 0), (float("nan"), 0, 1)):
                    try:
                        video_demuxer.SeekFrame(value, mode, criteria)
                    except (ValueError, TypeError):
                        pass
                    else:
                        raise AssertionError("Invalid seek accepted")
            if use_provider:
                assert not source.closed, "Demuxer closed caller-owned input"
    print("SEEK_PASS", path.name)


def check_seek_variants(directory):
    for extension in ("mp4", "mkv"):
        for rate, offset, bframes in ((Fraction(24), 0, 3), (Fraction(30000, 1001), 150, 3),
                                     (Fraction(24), 48, 0)):
            path = Path(directory) / f"seek-{rate.numerator}-{offset}-{bframes}.{extension}"
            with av.open(str(path), "w") as container:
                stream = container.add_stream("libx264", rate=rate)
                stream.width, stream.height, stream.pix_fmt = 64, 48, "yuv420p"
                stream.options = {"bf": str(bframes), "b-adapt": "0", "g": "12", "sc_threshold": "0"}
                for index in range(48):
                    frame = av.VideoFrame(64, 48, "yuv420p")
                    frame.pts = offset + index
                    for plane, value in zip(frame.planes, (32 + index * 3, 128, 128)):
                        plane.update(bytes([value]) * plane.buffer_size)
                    for packet in stream.encode(frame):
                        container.mux(packet)
                for packet in stream.encode(None):
                    container.mux(packet)
            check_seeking(path)


def check_unsupported_codecs(directory):
    for codec, extension in (("mpeg4", "mp4"), ("ffv1", "mkv")):
        path = Path(directory) / f"unsupported-{codec}.{extension}"
        with av.open(str(path), "w") as container:
            stream = container.add_stream(codec, rate=24)
            stream.width, stream.height, stream.pix_fmt = 64, 48, "yuv420p"
            frame = av.VideoFrame(64, 48, "yuv420p")
            for plane in frame.planes:
                plane.update(bytes([128]) * plane.buffer_size)
            for packet in list(stream.encode(frame)) + list(stream.encode(None)):
                container.mux(packet)
        try:
            with demuxer(path):
                raise AssertionError(f"Unsupported codec accepted: {codec}")
        except ValueError as error:
            assert f"Cannot decode '{codec}' using rocDecode" in str(error)
    print("UNSUPPORTED_CODECS_PASS")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=Path, required=True)
    args = parser.parse_args()
    if not args.input.is_file():
        print(f"Skipping pyav_regression_test: test media unavailable: {args.input}")
        sys.exit(77)
    with demuxer(args.input) as video_demuxer:
        assert video_demuxer.GetBitDepth() > 0
        codec_ids = {"h264": 27, "hevc": 173, "vp9": 167, "av1": 225}
        assert video_demuxer.GetCodecId() == codec_ids[video_demuxer._stream.codec_context.codec.canonical_name]
        packet = video_demuxer.DemuxFrame()
        saved = packet_data(packet)
        video_demuxer.DemuxFrame()
    assert packet_data(packet) == saved, "Packet storage expired after reading/closing"
    video_demuxer.close()
    try:
        video_demuxer.DemuxFrame()
    except ValueError:
        pass
    else:
        raise AssertionError("Closed demuxer accepted a read")
    check_gpu_decode(args.input)
    check_gpu_decode(args.input, timestamp=2.5)
    with tempfile.TemporaryDirectory() as directory:
        check_seek_variants(directory)
        check_unsupported_codecs(directory)
    print("PYAV_REGRESSIONS_PASS")


if __name__ == "__main__":
    main()
