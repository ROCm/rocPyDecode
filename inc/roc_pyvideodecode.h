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

// include here: /opt/rocm/include/rocdecode                        [rocdecode.h rocparser.h roc_video_dec.h video_demuxer.h << CLASS demux inside]
// CPP here:     /opt/rocm/share/rocdecode/utils/rocvideodecode/    [roc_video_dec.cpp roc_video_dec.h << CLASS in cpp]
 
//
// AMD Video Decoder Python Interface class
//
class pyRocVideoDecoder : public RocVideoDecoder {
    public:
        pyRocVideoDecoder(int device_id, rocDecVideoCodec codec, bool force_zero_latency = false,
                          const Rect *p_crop_rect = nullptr, int max_width = 0, int max_height = 0,
                          uint32_t clk_rate = 1000) : RocVideoDecoder(device_id, (OutputSurfaceMemoryType)0, codec, force_zero_latency,
                          p_crop_rect, false, max_width, max_height, clk_rate ){}
         
        // for pyhton binding
        int wrapper_DecodeFrame(PacketData& packet);
    
        // for pyhton binding
        py::object wrapper_GetFrame(PacketData& packet);

        // for pyhton binding
        py::object wrapper_ReleaseFrame(PacketData& packet /*pts*/, py::array_t<bool>& b_flushing_in);
      
        // for pyhton binding
        py::object wrapper_GetDeviceinfo(py::object &device_name_in, py::object &gcn_arch_name_in, py::object &pci_bus_id_in, py::object &pci_domain_id_in, py::object &pci_device_id_in);
        
        // for pyhton binding
        py::object wrapper_SaveFrameToFile(py::object& output_file_name_in,py::array_t<uint64_t>& surf_mem_adrs, py::array_t<uint8_t>& surface_info_adrs);

        // for pyhton binding
        py::object wrapper_GetOutputSurfaceInfoAdrs(OutputSurfaceInfo& surface_adrs, py::array_t<uint8_t>& surface_info_adrs);
 
        // for pyhton binding
        py::object wrapper_GetNumOfFlushedFrames();
        
        // added for python binding     
        ReconfigParams py_reconfig_params;         
};

 
