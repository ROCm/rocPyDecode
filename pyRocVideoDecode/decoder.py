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

import rocPyDecode as rocpydec
import rocPyDecode.decTypes as dectypes
import numpy as np


def GetRocDecCodecID(codec_id) -> dectypes.rocDecVideoCodec:
    rocCodecId = rocpydec.AVCodec2RocDecVideoCodec(codec_id)
    return rocCodecId


def GetRectangle(crop_rect: dict) -> rocpydec.Rect:
    p_crop_rect = rocpydec.Rect()
    if (crop_rect is not None):
        p_crop_rect.left = crop_rect[0]
        p_crop_rect.top = crop_rect[1]
        p_crop_rect.right = crop_rect[2]
        p_crop_rect.bottom = crop_rect[3]
    return p_crop_rect


def GetOutputSurfaceInfo():
    surf_info_struct = rocpydec.OutputSurfaceInfo()
    return surf_info_struct


class decoder(object):
    def __init__(
            self,
            device_id,
            mem_type,
            codec,
            b_force_zero_latency,
            crop_rect,
            max_width,
            max_height,
            clk_rate):
        p_crop_rect = GetRectangle(crop_rect)
        self.viddec = rocpydec.PyRocVideoDecoder(
            device_id,
            mem_type,
            codec,
            b_force_zero_latency,
            p_crop_rect,
            max_width,
            max_height,
            clk_rate)

    def GetGpuInfo(self):
        return self.viddec.GetDeviceinfo()

    def DecodeFrame(self, packet) -> int:
        return self.viddec.DecodeFrame(packet)

    def GetFrame(self, packet):
        pts = self.viddec.GetFrame(packet)
        return pts

    def GetFrameRgb(self, packet, rgb_format):
        pts = self.viddec.GetFrameRgb(packet, rgb_format)
        if(pts == -1):
            print("Error: GetFrameRgb returned failure.\n")
        return pts

    def GetWidth(self) -> int:
        return self.viddec.GetWidth()

    def GetHeight(self) -> int:
        return self.viddec.GetHeight()

    def GetStride(self) -> int:
        return self.viddec.GetStride()

    def GetFrameSize(self) -> int:
        return self.viddec.GetFrameSize()

    def GetOutputSurfaceInfo(self):
        return self.viddec.GetOutputSurfaceInfo()

    def SaveFrameToFile(self, output_file_path, frame_adrs, surface_info):
        return self.viddec.SaveFrameToFile( output_file_path, frame_adrs, surface_info)

    def SaveRgbFrameToFile(self, output_file_path, frame_adrs, width, height, rgb_format, surface_info):
        return self.viddec.SaveRgbFrameToFile(output_file_path, frame_adrs, width, height, rgb_format, surface_info)

    def SaveTensorToFile(self, output_file_path, frame_adrs, width, height, rgb_format, surface_info):
        return self.viddec.SaveTensorToFile(output_file_path, frame_adrs, width, height, rgb_format, surface_info)

    def ReleaseFrame(self, packet):
        self.viddec.ReleaseFrame(packet)
        return

    def GetNumOfFlushedFrames(self):
        return self.viddec.GetNumOfFlushedFrames()

    def InitMd5(self):
        return self.viddec.InitMd5()

    def UpdateMd5ForFrame(self, frame_adrs: np.uint64, surface_info: np.uint8):
        return self.viddec.UpdateMd5ForFrame(frame_adrs, surface_info)

    def FinalizeMd5(self):
        digest = np.zeros(16, np.uint8)
        self.viddec.FinalizeMd5(digest.ctypes.data)
        return digest
