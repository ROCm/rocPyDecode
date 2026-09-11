# Copyright (c) 2018 - 2023 Advanced Micro Devices, Inc. All rights reserved.
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
import argparse
from inspect import getmembers, isfunction
import sys

print('rocPyDecode Demuxer')
rocpydecodeDemuxer = getmembers(dmx, isfunction)
for i in range(len(rocpydecodeDemuxer)):
    print(rocpydecodeDemuxer[i])


def test_all_apis(input_file_path):
    stream = dmx.stream_provider(input_file_path)
    stream_provider_obj = stream.GetFileStreamProvider()
    demuxer_from_path = dmx.demuxer(input_file_path)
    demuxer_from_stream = dmx.demuxer(stream)
    demuxer_from_path.GetCodecId()
    demuxer_from_path.GetBitDepth()
    demuxer_from_path.DemuxFrame()
    dummy_frame =  dummy_seek_mode = dummy_seek_criteria = 0
    demuxer_from_path.SeekFrame(dummy_frame, dummy_seek_mode, dummy_seek_criteria)
    print('rocPyDecode Demuxer test finished.')

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Arguments')
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

    test_all_apis(input_file_path)
