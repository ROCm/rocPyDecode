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

#include "roc_pyvideodecode.h"
 

using namespace std;

void pyRocVideoDecoderInitializer(py::module& m)
{
        py::class_<pyRocVideoDecoder> (m, "pyRocVideoDecoder")
        .def(py::init<int,rocDecVideoCodec,bool,const Rect *,int,int,uint32_t>(),
                    py::arg("device_id"), py::arg("codec"), py::arg("force_zero_latency"), 
                    py::arg("p_crop_rect"), py::arg("max_width"), py::arg("max_height"), py::arg("clk_rate"))
        .def("GetDeviceinfo",&pyRocVideoDecoder::wrapper_GetDeviceinfo)
        .def("DecodeFrame",&pyRocVideoDecoder::wrapper_DecodeFrame) 
        .def("GetFrame",&pyRocVideoDecoder::wrapper_GetFrame)
        .def("SaveFrameToFile",&pyRocVideoDecoder::wrapper_SaveFrameToFile)
        .def("ReleaseFrame",&pyRocVideoDecoder::wrapper_ReleaseFrame)
        .def("GetOutputSurfaceInfoAdrs",&pyRocVideoDecoder::wrapper_GetOutputSurfaceInfoAdrs)
        .def("GetNumOfFlushedFrames",&pyRocVideoDecoder::wrapper_GetNumOfFlushedFrames);
}

void pyRocVideoDecoder::initConfigStructure() {
    configInfo.reset(new ConfigInfo());    
    configInfo.get()->device_name = std::string("");
    configInfo.get()->gcn_arch_name = std::string("");
    configInfo.get()->pci_bus_id = 0;
    configInfo.get()->pci_domain_id = 0;
    configInfo.get()->pci_device_id = 0;
}

int pyRocVideoDecoder::wrapper_DecodeFrame(PacketData& packet) {

    return DecodeFrame((u_int8_t*) packet.frame_adrs, (size_t) packet.frame_size, packet.pkt_flags, packet.frame_pts);    
}
 
// for pyhton binding
py::object pyRocVideoDecoder::wrapper_GetFrame(PacketData& packet) {
   
    packet.frame_adrs = (uintptr_t) GetFrame(&packet.frame_pts);   
    return py::cast(packet.frame_pts);
}

// for python binding (can not move it to header for py)
py::object pyRocVideoDecoder::wrapper_GetNumOfFlushedFrames() { 
    int32_t ret = GetNumOfFlushedFrames();
    return py::cast(ret);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_ReleaseFrame(PacketData& packet, py::array_t<bool>& b_flushing_in) {
  
    bool b_flushing = false;
    memcpy( &b_flushing, b_flushing_in.mutable_data(), sizeof(bool));
    bool ret = ReleaseFrame(packet.frame_pts, b_flushing);     
    return py::cast(ret);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_SaveFrameToFile(std::string& output_file_name_in, py::array_t<uint64_t>& surf_mem_adrs, py::array_t<uint8_t>& surface_info_adrs) {
 
     std::string output_file_name = output_file_name_in.c_str();
    uint64_t surf_mem;
    OutputSurfaceInfo surf_info;
    memcpy(&surf_mem, surf_mem_adrs.mutable_data(), sizeof(uint64_t)); 
    memcpy(&surf_info, surface_info_adrs.mutable_data(), sizeof(OutputSurfaceInfo)); 
    
    SaveFrameToFile(output_file_name, (void *)surf_mem, &surf_info);

    return py::cast<py::none>(Py_None);
}
 

// for pyhton binding
std::shared_ptr<ConfigInfo> pyRocVideoDecoder::wrapper_GetDeviceinfo() {

    GetDeviceinfo(configInfo.get()->device_name, configInfo.get()->gcn_arch_name, configInfo.get()->pci_bus_id, configInfo.get()->pci_domain_id, configInfo.get()->pci_device_id);
    return configInfo; 
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_GetOutputSurfaceInfoAdrs(OutputSurfaceInfo& surface_adrs, py::array_t<uint8_t>& surface_info_adrs) {
    
    OutputSurfaceInfo *l_surface_info;
    bool ret = GetOutputSurfaceInfo(&l_surface_info);

    if (ret){
        surface_info_adrs.resize({sizeof(OutputSurfaceInfo)}, false);
        memcpy(surface_info_adrs.mutable_data(), l_surface_info, sizeof(OutputSurfaceInfo)); 
		surface_adrs = *l_surface_info;
    }
    
    return py::cast(ret);
}
 