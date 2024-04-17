/*
Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

#include "roc_video_dec.h"
#include "roc_pydecode.h"

//
// AMD Video Decoder Python Interface class
//
class PyRocVideoDecoder : public RocVideoDecoder {

    public:
        PyRocVideoDecoder(int device_id, rocDecVideoCodec codec, bool force_zero_latency = false,
                          const Rect *p_crop_rect = nullptr, int max_width = 0, int max_height = 0,
                          uint32_t clk_rate = 1000) : RocVideoDecoder(device_id, (OutputSurfaceMemoryType)0, codec, force_zero_latency,
                          p_crop_rect, false, max_width, max_height, clk_rate){ InitConfigStructure(); }
         
        // for python binding
        int PyDecodeFrame(PyPacketData& packet);
    
        // for python binding
        py::object PyGetFrame(PyPacketData& packet);

        // for python binding
        py::object PyReleaseFrame(PyPacketData& packet, py::array_t<bool>& b_flushing_in);
      
        // for python binding
        std::shared_ptr<ConfigInfo> PyGetDeviceinfo();
        
        // for python binding
        py::object PySaveFrameToFile(std::string& output_file_name_in, uintptr_t& surf_mem, uintptr_t& surface_info);

        // for python binding
        uintptr_t PyGetOutputSurfaceInfo();
 
        // for python binding
        py::object PyGetNumOfFlushedFrames();    

    private:
        std::shared_ptr <ConfigInfo> configInfo;
        void InitConfigStructure();
};

 
