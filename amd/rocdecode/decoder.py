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
import numpy as np    


class decoder(object):
    def __init__(self, device_id: int, out_mem_type: roctypes.OutputSurfaceMemoryType, codec: roctypes.rocDecVideoCodec, b_force_zero_latency: bool, 
                 p_crop_rect: rocpydec.Rect, b_extract_sei_messages=False, max_width=0, max_height=0, clk_rate=0):
         self.viddec = rocpydec.pyRocVideoDecoder( device_id, out_mem_type, codec, b_force_zero_latency, p_crop_rect, b_extract_sei_messages,max_width, max_height, clk_rate)

    def Get_GPU_Info(self):
        self.device_name =  np.zeros(100,str)
        self.gcn_arch_name = np.zeros(100,str)
        self.pci_bus_id = np.array(1)
        self.pci_domain_id = np.array(1)
        self.pci_device_id = np.array(1)
        self.viddec.GetDeviceinfo(ctypes.c_void_p(self.device_name.ctypes.data), 
                                  ctypes.c_void_p(self.gcn_arch_name.ctypes.data), 
                                  ctypes.c_void_p(self.pci_bus_id.ctypes.data), 
                                  ctypes.c_void_p(self.pci_domain_id.ctypes.data), 
                                  ctypes.c_void_p(self.pci_device_id.ctypes.data))
        return [self.device_name, self.gcn_arch_name, self.pci_bus_id, self.pci_domain_id, self.pci_device_id]

    def SetReconfigurationParams(self, b_flush, b_dump_output_frames, output_file_path):
        output_file_name = ctypes.c_void_p(output_file_path.ctypes.data) 
        self.viddec.SetReconfigParams( ctypes.c_void_p(b_flush.ctypes.data), ctypes.c_void_p(b_dump_output_frames.ctypes.data), output_file_name)
        return output_file_name

    def InitMd5(self):
        self.viddec.InitMd5()

    def DecodeFrame(self, frame_adrs: np.uint64, frame_size: np.int64, pkg_flags: int, frame_pts: np.int64)->int:
        return self.viddec.DecodeFrame(frame_adrs[0], frame_size[0], pkg_flags, frame_pts[0])

    def GetFrameAddress(self, frame_pts:  np.int64, frame_adrs: np.uint64):
        self.viddec.GetFrameAddress(frame_pts, frame_adrs)    

    def GetOutputSurfaceInfoAdrs(self, surface_info_struct):
        surface_info_adrs = np.ndarray(shape=(0), dtype=np.uint8)
        ret = self.viddec.GetOutputSurfaceInfoAdrs(surface_info_struct, surface_info_adrs)
        return [ret, surface_info_adrs]

    def UpdateMd5ForFrame(self, frame_adrs: np.uint64, surface_info_adrs: np.uint8):
        self.viddec.UpdateMd5ForFrame(frame_adrs[0], surface_info_adrs)

    def SaveFrameToFile(self, output_file_name, frame_adrs: np.uint64, surface_info_adrs: np.uint8 ):
        return self.viddec.SaveFrameToFile( output_file_name, frame_adrs, surface_info_adrs)
    
    def ReleaseFrame(self, frame_pts: np.int64, b_flush: bool):
        # b_t = np.ndarray(shape=(1), dtype=np.uint8)
        self.viddec.ReleaseFrame(frame_pts, b_flush)

    def GetNumOfFlushedFrames(self):
        return self.viddec.GetNumOfFlushedFrames()
    
    def FinalizeMd5(self):
        digest = np.zeros(16,np.uint8)
        self.viddec.FinalizeMd5(ctypes.c_void_p(digest.ctypes.data))
        return digest
