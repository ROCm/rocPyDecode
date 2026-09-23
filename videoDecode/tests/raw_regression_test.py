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
import importlib.util
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
    if importlib.util.find_spec("av") is not None:
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

    if importlib.util.find_spec("av") is not None:
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
