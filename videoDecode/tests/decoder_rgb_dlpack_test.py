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


def test_rgb_dlpack(input_file_path):
    demuxer = dmx.demuxer(input_file_path)
    codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
    decoder = dec.decoder(codec_id, mem_type=OUT_SURFACE_MEM_DEV_COPIED, b_force_zero_latency=True)

    while True:
        packet = demuxer.DemuxFrame()
        for i in range(decoder.DecodeFrame(packet)):
            if decoder.GetFrameRgb(packet, rgb_format=3) == -1:
                continue

            # interleaved 8-bit RGB surface: shape [H, W, 3], 3 bytes/pixel, 1 byte/channel.
            buf = packet.ext_buf[0]
            assert buf.dtype == "|u1" # unsigned 8-bit integer, "|u1" == uint8
            assert len(buf.shape) == 3 and buf.shape[2] == 3
            assert buf.strides[1:] == (3, 1)

            decoder.ReleaseFrame(packet)
            print('rocPyDecode RGB DLPack test finished.')
            return

        if packet.bitstream_size <= 0:
            break

    raise RuntimeError('no RGB frame decoded')


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
