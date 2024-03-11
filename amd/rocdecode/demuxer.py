# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

import rocPyDecode as rocpydec              # rocpydecode main module 
import rocPyDecode.decTypes as roctypes
# import ctypes 
import numpy as np    


class demuxer(object):
    def __init__(self, input_file_path: str):
        print ("\rocPyDemuxer Constructor..\n") # to be removed: Essam
        
        self.vidmux = rocpydec.usrVideoDemuxer(input_file_path)

    def GetCodec_ID(self)-> roctypes.AVCodecID:
        return self.vidmux.GetCodec_ID() 

    def DemuxFrame(self):
        frame_adrs = np.ndarray(shape=(0), dtype=np.uint64) # one uint64 storage (carries address)
        frame_size = np.ndarray(shape=(0), dtype=np.int64)  # one int64  storage (carries int value)
        frame_pts  = np.ndarray(shape=(0), dtype=np.int64)  # one int64  storage (carries int value)
        return [self.vidmux.DemuxFrame(frame_adrs, frame_size, frame_pts), frame_adrs, frame_size, frame_pts]
