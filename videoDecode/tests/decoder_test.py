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

import numpy as np
import rocpydecode
from pyRocVideoDecode.decoder import GetOutputFormat, GetRocDecCodecID, GetRectangle, GetDim, GetOutputSurfaceInfo, GetRocPyDecPacket
import pyRocVideoDecode.demuxer as dmx
import pyRocVideoDecode.decoder as dec
import argparse

import pyRocVideoDecode.decoder as decoders

from inspect import getmembers, isfunction

print('rocPyDecode Decoders')
rocpydecodeDecoders = getmembers(decoders, isfunction)
for i in range(len(rocpydecodeDecoders)):
    print(rocpydecodeDecoders[i])


parser = argparse.ArgumentParser(
    description='PyRocDecode Video Decode Arguments')
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
    exit()

input_file_path = args.input

# test all APIs
codec_id = GetRocDecCodecID("h264")
output_format = GetOutputFormat(0)
crop_rect = GetRectangle((0, 0, 1920, 1080))
resize_dim = GetDim((640, 360))
surface_info = GetOutputSurfaceInfo()
demuxer = dmx.demuxer(input_file_path)
codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
decoder = dec.decoder(codec_id,0,1)
gpu_info = decoder.GetGpuInfo()
buffer = np.zeros((1080 * 1920 * 3,), dtype=np.uint8)
packet = demuxer.DemuxFrame()
decoder.DecodeFrame(packet)
decoder.GetFrameYuv(packet, separate_planes=False)
decoder.GetFrameRgb(packet, rgb_format=0)
GetRocPyDecPacket(0, size=buffer.size, buffer=buffer)
decoder.GetWidth()
decoder.GetHeight()
decoder.GetStride()
decoder.GetFrameSize()
decoder.GetOutputSurfaceInfo()
decoder.GetResizedOutputSurfaceInfo()
decoder.GetNumOfFlushedFrames()
decoder.AddDecoderSessionOverHead(session_id=1, duration=123456)
decoder.GetDecoderSessionOverHead(session_id=1)
decoder.IsCodecSupported(device_id=0, codec_id=codec_id, bit_depth=8)
decoder.GetBitDepth()
decoder.SaveFrameToFile("outfile.yuv", packet.frame_adrs)
decoder.ReleaseFrame(packet)

# Test AVCodecString2RocDecVideoCodec
try:
    codec_id = rocpydecode.AVCodecString2RocDecVideoCodec("h264")
except Exception as e:
    print("AVCodecString2RocDecVideoCodec failed:", e)

# Test AVCodec2RocDecVideoCodec
try:
    avcodec_id = 27
    codec_enum = rocpydecode.AVCodec2RocDecVideoCodec(avcodec_id)
except Exception as e:
    print("AVCodec2RocDecVideoCodec failed:", e)

# Test GetRocPyDecPacket
try:
    # Create a dummy buffer with arbitrary bytes
    data = np.frombuffer(b'\x00' * 128, dtype=np.uint8)
    packet = rocpydecode.GetRocPyDecPacket(0, 128, data)
except Exception as e:
    print("GetRocPyDecPacket failed:", e)
