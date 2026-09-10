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

import pyRocVideoDecode.demuxer as dmx
import pyRocVideoDecode.decoder as dec
from pyRocVideoDecode.types import OUT_SURFACE_MEM_DEV_COPIED
import argparse
import sys


def test_rgb_dlpack(input_file_path, decoder_class=dec.decoder, mem_type=OUT_SURFACE_MEM_DEV_COPIED):
    for fmt in range(1, 9):
        demuxer = dmx.demuxer(input_file_path)
        codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
        decoder = decoder_class(codec_id, mem_type=mem_type, b_force_zero_latency=True)
        while True:
            packet = demuxer.DemuxFrame()
            if decoder.DecodeFrame(packet):
                assert decoder.GetFrameRgb(packet, fmt) != -1
                buf = packet.ext_buf[0]
                channels = 4 if fmt >= 5 else 3
                assert buf.dtype == ("|u2" if fmt % 2 == 0 else "|u1")
                assert buf.shape == (decoder.GetHeight(), decoder.GetWidth(), channels)
                assert buf.strides[1:] == (channels, 1)
                decoder.ReleaseFrame(packet)
                break
            if packet.bitstream_size <= 0:
                raise RuntimeError("no RGB frame decoded")
    print("rocPyDecode RGB DLPack formats 1–8 passed.")


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='PyRocDecode RGB DLPack Test Arguments')
    parser.add_argument(
        '-i',
        '--input',
        type=str,
        help='Input File Path - required',
        required=True)
    try:
        args = parser.parse_args()
    except SystemExit as e:
        print(f"Error: {e}. Please check the input arguments and try again.")
        sys.exit(1)

    input_file_path = args.input

    test_rgb_dlpack(input_file_path)
