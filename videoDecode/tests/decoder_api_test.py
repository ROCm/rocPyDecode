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

import rocpydecode
from rocpydecode import DLPackPyTensor
import argparse
import sys
import tempfile
from pathlib import Path
from pyRocVideoDecode._pyav import require_av
from pyRocVideoDecode.demuxer import demuxer
from pyRocVideoDecode.decoder import decoder, GetRocDecCodecID
 
parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Input Arguments')
parser.add_argument(
    '-i',
    '--input',
    type=str,
    help='Input File Path - required',
    required=True)
args = parser.parse_args()

input_file_path = args.input

try:
    require_av()
except ImportError as error:
    print(f"Skipping decoder_api_test: {error}")
    sys.exit(77)
if not Path(input_file_path).is_file():
    print(f"Skipping decoder_api_test: test media unavailable: {input_file_path}")
    sys.exit(77)

def first_frame(gpu, mux):
    while True:
        packet = mux.DemuxFrame()
        if gpu.DecodeFrame(packet) > 0:
            return packet
        assert not packet.end_of_stream, "API smoke test did not decode a frame"


# Exercise the same Python demuxer and decoder calls used by applications.
with demuxer(input_file_path) as mux:
    codec = GetRocDecCodecID(mux.GetCodecId())
    gpu = decoder(codec, mem_type=1)
    assert gpu.IsCodecSupported(0, codec, mux.GetBitDepth())
    packet = first_frame(gpu, mux)
    assert gpu.GetFrameRgb(packet, 3) != -1
    assert packet.ext_buf[0].shape == (gpu.GetHeight(), gpu.GetWidth(), 3)
    assert packet.ext_buf[0].__dlpack_device__() == (10, 0)
    surface = gpu.GetOutputSurfaceInfo()
    assert surface
    dimensions = (32, 48) if (gpu.GetWidth(), gpu.GetHeight()) == (64, 48) else (64, 48)
    assert gpu.ResizeFrame(packet, dimensions, surface)
    assert gpu.GetResizedOutputSurfaceInfo()
    assert gpu.GetGpuInfo().device_name
    assert gpu.GetNumOfFlushedFrames() >= 0 and gpu.GetBitDepth() >= 8
    gpu.ReleaseFrame(packet)
    gpu.SetReconfigParams(0, "")
    del gpu
print("Python API smoke test decoded, resized, and released a GPU frame.")

# Additional native checks are available in Debug builds with BUILD_TESTING=ON.
if hasattr(rocpydecode, "Test_PyReconfigureFlushCallback"):
    rocpydecode.TestAll_roc_pybuffer()
    rocpydecode.Test_DLPack()
    with tempfile.TemporaryDirectory(prefix="rocpydecode-flush-") as output_directory:
        for scenario in range(3):
            with demuxer(input_file_path) as reference_mux, demuxer(input_file_path) as test_mux:
                reference = decoder(codec, mem_type=1)
                subject = decoder(codec, mem_type=1)
                reference_packet = first_frame(reference, reference_mux)
                packet = first_frame(subject, test_mux)
                assert reference_packet.frame_pts == packet.frame_pts
                rocpydecode.Test_PyReconfigureFlushCallback(
                    reference.viddec, subject.viddec, scenario, output_directory)
                del reference, subject
    rocpydecode.Test_CalculateRgbImageSize()
    DLPackPyTensor.test_all()
    print("Native buffer, DLPack, flush, and RGB size checks passed.")
print("rocPyDecode API tests completed successfully.")
