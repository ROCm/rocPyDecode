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

import pyRocVideoDecode.decodercpu as decoderscpu
from inspect import getmembers, isfunction
import rocpydecode.decTypes as dectypes
import numpy as np
from pyRocVideoDecode.decodercpu import decodercpu, GetOutputFormat, GetRocDecCodecID, GetRectangle, GetDim, GetOutputSurfaceInfo, GetRocPyDecPacket
import pyRocVideoDecode.demuxer as dmx
import pyRocVideoDecode.decodercpu as dec
import argparse
from pathlib import Path
import tempfile

parser = argparse.ArgumentParser(
    description='PyRocDecode Video Decode Arguments')
parser.add_argument(
    '-i',
    '--input',
    type=str,
    help='Input File Path - required',
    required=True)

args = parser.parse_args()

input_file_path = args.input

print('rocPyDecode DecodersCPU')
rocpydecodeDecoders = getmembers(decoderscpu, isfunction)
for i in range(len(rocpydecodeDecoders)):
    print(rocpydecodeDecoders[i])

# test all APIs
codec_id = GetRocDecCodecID("h264")
output_format = GetOutputFormat(0)
crop_rect = GetRectangle((0, 0, 1920, 1080))
resize_dim = GetDim((640, 360))
surface_info = GetOutputSurfaceInfo()
demuxer = dmx.demuxer(input_file_path)
codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
decoder = dec.decodercpu(codec_id, 0, 1, crop_rect=(0, 0, 0, 0))
gpu_info = decoder.GetGpuInfo()
buffer = np.zeros((1080 * 1920 * 3,), dtype=np.uint8)
while True:
    packet = demuxer.DemuxFrame()
    if decoder.DecodeFrame(packet) > 0:
        break
    if packet.bitstream_size <= 0:
        raise AssertionError("CPU API smoke test did not decode a frame")
assert decoder.GetFrameYuv(packet, separate_planes=False) != -1
assert packet.frame_adrs, "CPU API smoke test did not retrieve a frame"
try:
    decoder.GetFrameRgb(packet, rgb_format=0)
except ValueError:
    pass
else:
    raise AssertionError("Native YUV is not an RGB output format")
GetRocPyDecPacket(0, size=buffer.size, buffer=buffer)
assert decoder.GetWidth() > 0
assert decoder.GetHeight() > 0
assert decoder.GetStride() > 0
assert decoder.GetFrameSize() > 0
surface_info = decoder.GetOutputSurfaceInfo()
assert surface_info
assert decoder.ResizeFrame(packet, (640, 360), surface_info)
assert decoder.GetResizedOutputSurfaceInfo()
decoder.GetNumOfFlushedFrames()
decoder.AddDecoderSessionOverHead(session_id=1, duration=123456)
decoder.GetDecoderSessionOverHead(session_id=1)
decoder.IsCodecSupported(device_id=0, codec_id=codec_id, bit_depth=8)
decoder.GetBitDepth()
with tempfile.TemporaryDirectory() as directory:
    output = Path(directory) / "frame.yuv"
    decoder.SaveFrameToFile(str(output), packet.frame_adrs)
    assert output.stat().st_size > 0, "CPU API smoke test saved an empty frame"
decoder.ReleaseFrame(packet)
print("CPU API smoke test decoded, resized, saved, and released a frame.")

# The PyRocVideoDecoderCpu entry point accepts a bound Dim for resizing.
import rocpydecode as native
with dmx.demuxer(input_file_path) as legacy_mux:
    legacy = native.PyRocVideoDecoderCpu(codec=codec_id)
    while True:
        legacy_packet = legacy_mux.DemuxFrame()
        if legacy.DecodeFrame(legacy_packet):
            break
        assert not legacy_packet.end_of_stream, "Legacy CPU API decoded no frame"
    assert legacy.GetFrameYuv(legacy_packet) != -1
    assert legacy.ResizeFrame(legacy_packet, resize_dim, legacy.GetOutputSurfaceInfo())
    assert legacy.GetResizedOutputSurfaceInfo()
    legacy.ReleaseFrame(legacy_packet)

# CPU conversion must handle its planar YUV output in both memory modes.
from decoder_rgb_dlpack_test import test_rgb_dlpack
for memory_type in (1, 2):
    test_rgb_dlpack(input_file_path, dec.decodercpu, memory_type, zero_latency=False)
