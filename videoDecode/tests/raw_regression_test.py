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

"""Exercise raw sample frame limits and failure exit codes using SDK media."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import ctypes
import hashlib
import rocpydecode as native
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from types import SimpleNamespace
from unittest.mock import patch
from pyRocVideoDecode.decoder import GetRocDecCodecID
from pyRocVideoDecode._pyav import require_av

try:
    av = require_av()
except ImportError:
    av = None


def pyav_version_boundaries():
    for version, supported in (("17.0.0", False), ("18.0.0", False),
                               ("18.1.0", True), ("18.2.0", True),
                               ("19.0.0", False), ("20.0.0", False)):
        module = SimpleNamespace(__version__=version)
        with patch.dict(sys.modules, {"av": module}):
            if supported:
                assert require_av() is module, version
            else:
                try:
                    require_av()
                except ImportError as error:
                    assert ">=18.1,<19" in str(error), error
                else:
                    raise AssertionError(f"Unsupported PyAV accepted: {version}")
    print("PyAV version boundary checks passed")


def codec_translation():
    codecs = ((1, "mpeg1video", "MPEG1"), (2, "mpeg2video", "MPEG2"),
              (7, "mjpeg", "JPEG"), (12, "mpeg4", "MPEG4"), (27, "h264", "AVC"),
              (139, "vp8", "VP8"), (167, "vp9", "VP9"), (173, "hevc", "HEVC"),
              (225, "av1", "AV1"))
    # GPU codec translation must work even when PyAV cannot be imported.
    with patch.dict(sys.modules, {"av": None}):
        for number, name, suffix in codecs:
            expected = getattr(native.decTypes.rocDecVideoCodec, "rocDecVideoCodec_" + suffix)
            for value in (number, name, expected):
                assert GetRocDecCodecID(value) == expected, value
        for alias, name in (("h265", "hevc"), ("mpeg1", "mpeg1video"), ("mpeg2", "mpeg2video")):
            assert GetRocDecCodecID(alias) == GetRocDecCodecID(name)
        for value in (-1, 0, 2**31, "unknown"):
            try:
                GetRocDecCodecID(value)
            except ValueError:
                pass
            else:
                raise AssertionError(f"Unsupported codec accepted: {value}")
    if av is not None:
        for number, name, _ in codecs:
            assert av.Codec(name, "r").id == number, name
    print("Codec translation with and without PyAV passed")


def run(sample, args, expected_frames=None, error=None):
    result = subprocess.run(
        [sys.executable, str(sample), *map(str, args)],
        capture_output=True, text=True, timeout=60,
    )
    output = result.stdout + result.stderr
    if error is not None:
        assert result.returncode > 0 and error in output, output
    else:
        assert result.returncode == 0, output
        counts = re.findall(r"Decoded (\d+) frames", output)
        assert counts == [str(expected_frames)], output


def packet_and_session_boundaries(media):
    codec = native.decTypes.rocDecVideoCodec.rocDecVideoCodec_AVC
    classes = [native.PyRocVideoDecoder]
    if av is not None:
        classes.append(native.PyRocVideoDecoderCpu)
    for cls in classes:
        decoder = cls(codec=codec, out_mem_type=1)
        other = cls(codec=codec, out_mem_type=1)
        if hasattr(decoder, "AddDecoderSessionOverHead"):
            keys = [-(2**31), -1, 0, 1, 2**31 - 1]
            for key in keys:
                assert decoder.GetDecoderSessionOverHead(key) == 0
                decoder.AddDecoderSessionOverHead(key, 1.25)
                decoder.AddDecoderSessionOverHead(key, 2.5)
                assert decoder.GetDecoderSessionOverHead(key) == 3.75
                assert other.GetDecoderSessionOverHead(key) == 0
            def accumulate(_):
                for _ in range(100):
                    decoder.AddDecoderSessionOverHead(1, 0.5)
            with ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(accumulate, range(4)))
            assert decoder.GetDecoderSessionOverHead(1) == 203.75
        max_packet_size = 2**31 - 1 if cls is getattr(native, "PyRocVideoDecoderCpu", None) else 2**32 - 1
        data = ctypes.create_string_buffer(1)
        for address in [0, ctypes.addressof(data)]:
            for size in [-(2**63), -1, max_packet_size + 1, max_packet_size + 2, 2**63 - 1]:
                packet = native.PyPacketData()
                packet.bitstream_adrs, packet.bitstream_size = address, size
                packet.pkt_flags, packet.frame_pts = 0, 0
                try:
                    decoder.DecodeFrame(packet)
                except ValueError:
                    pass
                else:
                    raise AssertionError(f"Accepted packet size {size}, address {address}")
                assert packet.pkt_flags == 0
        packet = native.PyPacketData()
        packet.bitstream_adrs, packet.bitstream_size = 0, 1
        packet.pkt_flags, packet.frame_pts = 0, 0
        try:
            decoder.DecodeFrame(packet)
        except ValueError:
            pass
        else:
            raise AssertionError("Accepted a non-empty packet with no address")
        packet.bitstream_size = 0
        assert decoder.DecodeFrame(packet) == 0  # Valid EOS still works.
        del decoder, other

    if av is not None:
        def packets(mux):
            result = []
            for _ in range(10000):
                packet = mux.DemuxFrame()
                if packet.bitstream_size == 0:
                    return result
                assert packet.bitstream_size > 0
                data = ctypes.string_at(packet.bitstream_adrs, packet.bitstream_size)
                result.append(hashlib.sha256(data).digest())
            raise AssertionError("Demuxer did not reach EOF")
        expected = packets(native.PyVideoDemuxer(str(media)))
        provider = native.PyFileStreamProvider(str(media))
        mux = native.PyVideoDemuxer(provider)
        assert expected and packets(mux) == expected
        assert provider.GetBufferSize() == 0
        del mux, provider
    print("Packet, session-key, and mapped-provider regressions passed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--media-dir", required=True, type=Path)
    args = parser.parse_args()
    pyav_version_boundaries()
    codec_translation()
    packet_and_session_boundaries(args.media_dir / "AMD_driving_virtual_20-H264.264")
    sample = Path(__file__).resolve().parents[1] / "samples/rocdecode/videodecoderaw.py"
    for codec in ("264", "265"):
        media = args.media_dir / f"AMD_driving_virtual_20-H{codec}.{codec}"
        for count in (1, 2, 10):
            run(sample, ["-i", media, "-f", count], expected_frames=count)
    with tempfile.TemporaryDirectory() as directory:
        for codec in ("264", "265"):
            empty = Path(directory) / f"empty.{codec}"
            empty.touch()
            run(sample, ["-i", empty], error="No frames decoded")
            for count in (0, -2):
                run(sample, ["-i", empty, "-f", count], error="--frames must be")
    print("Raw video frame-limit and empty-input regressions passed")


if __name__ == "__main__":
    main()
