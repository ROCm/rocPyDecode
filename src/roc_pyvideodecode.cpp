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

void PyRocVideoDecoderInitializer(py::module& m) {
        py::class_<PyRocVideoDecoder> (m, "PyRocVideoDecoder")
        .def(py::init<int,rocDecVideoCodec,bool,const Rect *,int,int,uint32_t>(),
                    py::arg("device_id")=0, py::arg("codec")=rocDecVideoCodec_HEVC, py::arg("force_zero_latency")=false, 
                    py::arg("p_crop_rect")=nullptr, py::arg("max_width")=0, py::arg("max_height")=0, py::arg("clk_rate")=0)
        .def("GetDeviceinfo",&PyRocVideoDecoder::PyGetDeviceinfo)
        .def("DecodeFrame",&PyRocVideoDecoder::PyDecodeFrame) 
        .def("GetFrame",&PyRocVideoDecoder::PyGetFrame)
        .def("SaveFrameToFile",&PyRocVideoDecoder::PySaveFrameToFile)
        .def("SaveTensorToFile",&PyRocVideoDecoder::PySaveTensorToFile)
        .def("ReleaseFrame",&PyRocVideoDecoder::PyReleaseFrame)
        .def("GetOutputSurfaceInfo",&PyRocVideoDecoder::PyGetOutputSurfaceInfo)
        .def("GetNumOfFlushedFrames",&PyRocVideoDecoder::PyGetNumOfFlushedFrames)
        .def("InitMd5",&PyRocVideoDecoder::PyInitMd5)
        .def("FinalizeMd5",&PyRocVideoDecoder::PyFinalizeMd5)
        .def("UpdateMd5ForFrame",&PyRocVideoDecoder::PyUpdateMd5ForFrame)
        ;
}

void PyRocVideoDecoder::InitConfigStructure() {
    configInfo.reset(new ConfigInfo());    
    configInfo.get()->device_name = std::string("");
    configInfo.get()->gcn_arch_name = std::string("");
    configInfo.get()->pci_bus_id = 0;
    configInfo.get()->pci_domain_id = 0;
    configInfo.get()->pci_device_id = 0;
}

int PyRocVideoDecoder::PyDecodeFrame(PacketData& packet) {
 
    int decoded_frame_count = DecodeFrame((u_int8_t*) packet.frame_adrs, static_cast<size_t>(packet.frame_size), packet.pkt_flags, packet.frame_pts);    

    // Load DLPack Tensor
    if(packet.frame_adrs && decoded_frame_count) {
        uint32_t width = GetWidth();
        uint32_t height = GetHeight();        
        std::vector<size_t> shape{ (size_t)(height * 1.5), width};
        std::vector<size_t> stride{ size_t(width), 1};        
        packet.extBuf.get()->LoadDLPack(shape, stride, "|u1", (void *)packet.frame_adrs );
    }

    return decoded_frame_count;
}
 
// for python binding
py::object PyRocVideoDecoder::PyGetFrame(PacketData& packet) {
    int64_t pts = packet.frame_pts;
    packet.frame_adrs = reinterpret_cast<std::uintptr_t>(GetFrame(&pts));   
    packet.frame_pts = pts;
    return py::cast(packet.frame_pts);
}

// for python binding (can not move it to header for py)
py::object PyRocVideoDecoder::PyGetNumOfFlushedFrames() { 
    int32_t ret = GetNumOfFlushedFrames();
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoder::PyReleaseFrame(PacketData& packet) {  
    bool ret = ReleaseFrame(packet.frame_pts, true);     
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoder::PySaveFrameToFile(std::string& output_file_name_in, uintptr_t& surf_mem, uintptr_t& surface_info) {     
    std::string output_file_name = output_file_name_in.c_str();   
    if(surf_mem && surface_info) {
        SaveFrameToFile(output_file_name, (void *)surf_mem, reinterpret_cast<OutputSurfaceInfo*>(surface_info));
    }
    return py::cast<py::none>(Py_None);
}
 
// for python binding
py::object PyRocVideoDecoder::PySaveTensorToFile(std::string& output_file_name_in, uintptr_t& surf_mem, uintptr_t& surface_info) {
 
    std::string output_file_name = output_file_name_in.c_str();   
    if(surf_mem && surface_info) {

        OutputSurfaceInfo* si = reinterpret_cast<OutputSurfaceInfo*>(surface_info);
        si->mem_type = OUT_SURFACE_MEM_HOST_COPIED; // will not copy from D2H

        SaveFrameToFile(output_file_name, (void *)surf_mem, si);
    }
    return py::cast<py::none>(Py_None);
}

// for python binding
std::shared_ptr<ConfigInfo> PyRocVideoDecoder::PyGetDeviceinfo() {
    GetDeviceinfo(configInfo.get()->device_name, configInfo.get()->gcn_arch_name, configInfo.get()->pci_bus_id, configInfo.get()->pci_domain_id, configInfo.get()->pci_device_id);
    return configInfo; 
}

// for python binding
uintptr_t PyRocVideoDecoder::PyGetOutputSurfaceInfo() {
    OutputSurfaceInfo *l_surface_info;
    bool ret = GetOutputSurfaceInfo(&l_surface_info);
    if (ret) {
       return reinterpret_cast<std::uintptr_t>(l_surface_info);
    }
    return reinterpret_cast<std::uintptr_t>(nullptr);
}
 
// for pyhton binding
py::object PyRocVideoDecoder::PyInitMd5() {
    InitMd5();
    return py::cast<py::none>(Py_None);
}

// for pyhton binding
py::object PyRocVideoDecoder::PyUpdateMd5ForFrame(uintptr_t& surf_mem, uintptr_t& surface_info) {  
  
    if(surface_info && surf_mem)
        UpdateMd5ForFrame((void *)surf_mem, reinterpret_cast<OutputSurfaceInfo*>(surface_info));
        
    return py::cast<py::none>(Py_None);
}

// for pyhton binding
py::object PyRocVideoDecoder::PyFinalizeMd5(uintptr_t& digest_back) {    
    uint8_t * digest;
    FinalizeMd5(&digest);
    memcpy((uint8_t*)digest_back, digest,  sizeof(uint8_t)*16);
    return py::cast<py::none>(Py_None);
}

