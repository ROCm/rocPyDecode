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

"""Exercise the public Python demux/decode flow without NumPy or PyTorch."""
import argparse
from pathlib import Path
import sys
import rocpydecode
from pyRocVideoDecode._pyav import require_av
from pyRocVideoDecode.demuxer import demuxer
from pyRocVideoDecode.decoder import decoder, GetRocDecCodecID

parser = argparse.ArgumentParser(description="Demux video and decode YUV frames on the GPU")
parser.add_argument("-i", "--input", type=Path, required=True)
args = parser.parse_args()
try:
    require_av()
except ImportError as error:
    print(f"Skipping decoder_api_test: {error}")
    sys.exit(77)
if not args.input.is_file():
    print(f"Skipping decoder_api_test: test media unavailable: {args.input}")
    sys.exit(77)

video_demuxer = demuxer(args.input)
try:
    codec = GetRocDecCodecID(video_demuxer.GetCodecId())
    video_decoder = decoder(codec)
    assert video_decoder.IsCodecSupported(0, codec, video_demuxer.GetBitDepth())
    frame_count = 0
    while True:
        packet = video_demuxer.DemuxFrame()
        for _ in range(video_decoder.DecodeFrame(packet)):
            assert video_decoder.GetFrameYuv(packet) != -1
            assert video_decoder.GetFrameSize() > 0
            video_decoder.ReleaseFrame(packet)
            frame_count += 1
        if packet.end_of_stream:
            break
    assert frame_count > 0, "No video frames decoded"
    print(f"PYTHON_API_PASS: decoded and released {frame_count} YUV frames")
finally:
    video_demuxer.close()

# Native-only checks are compiled when BUILD_TESTING=ON, in either build type.
if hasattr(rocpydecode, "TestAll_roc_pybuffer"):
    rocpydecode.TestAll_roc_pybuffer()
    rocpydecode.Test_DLPack()
    rocpydecode.Test_CalculateRgbImageSize()
    rocpydecode.DLPackPyTensor.test_all()
    print("NATIVE_CHECKS_PASS")
