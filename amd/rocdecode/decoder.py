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
import ctypes 
# import numpy as np    


class decoder(object):
    def __init__(self, device_id: int, out_mem_type: roctypes.OutputSurfaceMemoryType, codec: roctypes.rocDecVideoCodec, b_force_zero_latency: bool, 
                 p_crop_rect: rocpydec.Rect, b_extract_sei_messages=False, max_width=0, max_height=0, clk_rate=0):
        print ("\rocPyDecode Constructor..\n") # to be removed: Essam
        
        self.viddec = rocpydec.pyRocVideoDecoder( device_id, out_mem_type, codec, b_force_zero_latency, p_crop_rect, b_extract_sei_messages,max_width, max_height, clk_rate)

    def Get_GPU_Info(self, device_name, gcn_arch_name, pci_bus_id, pci_domain_id, pci_device_id):
        self.viddec.GetDeviceinfo(ctypes.c_void_p(device_name.ctypes.data), ctypes.c_void_p(gcn_arch_name.ctypes.data), ctypes.c_void_p(pci_bus_id.ctypes.data), ctypes.c_void_p(pci_domain_id.ctypes.data), ctypes.c_void_p(pci_device_id.ctypes.data))

    def SetReconfigurationParams(self, b_flush, b_dump_output_frames, output_file_name):
        self.viddec.SetReconfigParams( ctypes.c_void_p(b_flush.ctypes.data), ctypes.c_void_p(b_dump_output_frames.ctypes.data), output_file_name)

    def InitMd5(self):
        self.viddec.InitMd5()