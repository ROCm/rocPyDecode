# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
import numpy as np

# self test buffer/dlpack
try:
    rocpydecode.RunAllSelfTests()
except Exception as e:
    print("RunAllSelfTests failed:", e)

# Test AVCodecString2RocDecVideoCodec
try:
    codec_id = rocpydecode.AVCodecString2RocDecVideoCodec("h264")
except Exception as e:
    print("AVCodecString2RocDecVideoCodec failed:", e)

# Test AVCodec2RocDecVideoCodec
try:
    # Replace with a real AVCodecID if you know the mapping, for example: 27 for H264
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
