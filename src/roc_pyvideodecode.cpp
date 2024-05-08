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
#include "colorspace_kernels.h"
#include <cstdint>
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

void PyRocVideoDecoderInitializer(py::module& m) {
        py::class_<PyRocVideoDecoder> (m, "PyRocVideoDecoder")
        .def(py::init<int,int,rocDecVideoCodec,bool,const Rect *,int,int,uint32_t>(),
                    py::arg("device_id") = 0, py::arg("out_mem_type") = 0, py::arg("codec") = rocDecVideoCodec_HEVC, py::arg("force_zero_latency") = false, 
                    py::arg("p_crop_rect") = nullptr, py::arg("max_width") = 0, py::arg("max_height") = 0, py::arg("clk_rate") = 1000)
        .def("GetDeviceinfo",&PyRocVideoDecoder::PyGetDeviceinfo)
        .def("DecodeFrame",&PyRocVideoDecoder::PyDecodeFrame) 
        .def("GetFrame",&PyRocVideoDecoder::PyGetFrame)
        .def("GetFrameRgb",&PyRocVideoDecoder::PyGetFrameRgb)
        .def("GetWidth",&PyRocVideoDecoder::PyGetWidth)
        .def("GetHeight",&PyRocVideoDecoder::PyGetHeight)
        .def("GetStride",&PyRocVideoDecoder::PyGetStride)
        .def("GetFrameSize",&PyRocVideoDecoder::PyGetFrameSize)
        .def("SaveFrameToFile",&PyRocVideoDecoder::PySaveFrameToFile)
        .def("SaveTensorToFile",&PyRocVideoDecoder::PySaveTensorToFile)
        .def("SaveRgbFrameToFile",&PyRocVideoDecoder::PySaveRgbFrameToFile)
        .def("ReleaseFrame",&PyRocVideoDecoder::PyReleaseFrame)
        .def("GetOutputSurfaceInfo",&PyRocVideoDecoder::PyGetOutputSurfaceInfo)
        .def("GetNumOfFlushedFrames",&PyRocVideoDecoder::PyGetNumOfFlushedFrames)
        .def("InitMd5",&PyRocVideoDecoder::PyInitMd5)
        .def("FinalizeMd5",&PyRocVideoDecoder::PyFinalizeMd5)
        .def("UpdateMd5ForFrame",&PyRocVideoDecoder::PyUpdateMd5ForFrame);
}

void PyRocVideoDecoder::InitConfigStructure() {
    configInfo.reset(new ConfigInfo());    
    configInfo.get()->device_name = std::string("");
    configInfo.get()->gcn_arch_name = std::string("");
    configInfo.get()->pci_bus_id = 0;
    configInfo.get()->pci_domain_id = 0;
    configInfo.get()->pci_device_id = 0;
}

PyRocVideoDecoder::~PyRocVideoDecoder() {
    // close tensor rgb file if still open, used in SAVE Tensor
    if( fp_tensor_rgb != nullptr) {
        fclose(fp_tensor_rgb);
        fp_tensor_rgb = nullptr;
    }
    // free host mem, used in SAVE Tensor
    if(hst_ptr_tensor_rgb != nullptr) {
        delete hst_ptr_tensor_rgb;
        hst_ptr_tensor_rgb = nullptr;
    }
    // free new RGB frame ptr if used
    if (frame_ptr_rgb != nullptr) {
        hipError_t hip_status = hipFree(frame_ptr_rgb);
        if (hip_status != hipSuccess) {
            std::cerr << "ERROR: hipFree failed! (" << hip_status << ")" << std::endl;
        }
        frame_ptr_rgb = nullptr;
    }
    if( post_process_class != nullptr ) {
        delete post_process_class;
        post_process_class = nullptr;
    }
}

int PyRocVideoDecoder::PyDecodeFrame(PyPacketData& packet) {
    int decoded_frame_count = DecodeFrame((u_int8_t*) packet.frame_adrs, static_cast<size_t>(packet.frame_size), packet.pkt_flags, packet.frame_pts);
    return decoded_frame_count;
}
 
// for python binding
py::object PyRocVideoDecoder::PyGetFrame(PyPacketData& packet) {
    int frame_size = GetFrameSize();
    int64_t pts = packet.frame_pts;
    packet.frame_adrs = reinterpret_cast<std::uintptr_t>(GetFrame(&pts));   
    packet.frame_pts = pts;
    // Load DLPack Tensor
    if(((uint8_t*) packet.frame_adrs != nullptr) && (frame_size > 0)) {
        uint32_t width = GetWidth();
        uint32_t height = GetHeight();    
        uint32_t surf_stride = GetSurfaceStride(); 
        std::string type_str((const char*)"|u1");
        std::vector<size_t> shape{ static_cast<size_t>(height * 1.5), static_cast<size_t>(width)}; // NV12
        std::vector<size_t> stride{ static_cast<size_t>(surf_stride), 1};
        packet.extBuf->LoadDLPack(shape, stride, type_str, (void *)packet.frame_adrs);
    }
    return py::cast(packet.frame_pts);
}

size_t PyRocVideoDecoder::CalculateRgbImageSize(OutputFormatEnum& e_output_format, OutputSurfaceInfo * p_surf_info) {
    size_t rgb_image_size = 0;
    int rgb_width = 0;
    if (p_surf_info->bit_depth == 8) {
        rgb_width = (p_surf_info->output_width + 1) & ~1; // has to be a multiple of 2 for hip colorconvert kernels
        rgb_image_size = ((e_output_format == bgr) || (e_output_format == rgb)) ? rgb_width * p_surf_info->output_height * 3 : rgb_width * p_surf_info->output_height * 4;
    } else {
        rgb_width = (p_surf_info->output_width + 1) & ~1;
        rgb_image_size = ((e_output_format == bgr) || (e_output_format == rgb)) ? rgb_width * p_surf_info->output_height * 3 : ((e_output_format == bgr48) || (e_output_format == rgb48)) ?
                                                rgb_width * p_surf_info->output_height * 6 : rgb_width * p_surf_info->output_height * 8;
    }
    return rgb_image_size;
}

// for python binding
py::object PyRocVideoDecoder::PyGetFrameRgb(PyPacketData& packet, int rgb_format) {
    OutputFormatEnum e_output_format = (OutputFormatEnum)rgb_format;
    // Get YUV Frame
    int64_t pts = packet.frame_pts;
    packet.frame_adrs = reinterpret_cast<std::uintptr_t>(GetFrame(&pts));
    packet.frame_pts = pts;
    // Load DLPack Tensor
    if((u_int8_t*)packet.frame_adrs != nullptr) {
        // get surface info
        OutputSurfaceInfo * surf_info = nullptr;
        GetOutputSurfaceInfo(&surf_info);
        if(surf_info == nullptr)
            return py::cast(-1); // ret failure
        // get/calc new rgb image size
        size_t rgb_image_size = CalculateRgbImageSize(e_output_format, surf_info);
        if(rgb_image_size <= 0)
            return py::cast(-1); // ret failure
        // allocate 'new' RGB image device-memory if wasn't
        if(frame_ptr_rgb == nullptr) {
            HIP_API_CALL(hipMalloc((void **)&frame_ptr_rgb, rgb_image_size));
            if(frame_ptr_rgb == nullptr)
                return py::cast(-1); // ret failure
        }
        // create new instance of post process class if not created
        if(post_process_class == nullptr) {
            post_process_class = new VideoPostProcess();
        }
        // use post process instance
        VideoPostProcess * post_proc = post_process_class;
        // Get Stream, and convert YUV 2 RGB
        post_proc->ColorConvertYUV2RGB((uint8_t*)packet.frame_adrs, surf_info, frame_ptr_rgb, e_output_format, GetStream());
        // save the rgb ptr
        packet.frame_adrs_rgb = reinterpret_cast<std::uintptr_t>(frame_ptr_rgb);
        // Load DLPack Tensor
        if((uint8_t*) packet.frame_adrs != nullptr) {
            uint32_t width = GetWidth();
            uint32_t height = GetHeight();
            uint32_t surf_stride = (width * (((e_output_format == bgr) || (e_output_format == rgb)) ? 3 : 4));
             std::string type_str((const char*)"|u1");
            std::vector<size_t> shape{ static_cast<size_t>(height), static_cast<size_t>(width)};
            std::vector<size_t> stride{ static_cast<size_t>(surf_stride), 1};
            packet.extBuf->LoadDLPack(shape, stride, type_str, (void *)frame_ptr_rgb);
        }
    }
    return py::cast(packet.frame_pts);
}

// for python binding (can not move it to header for py)
py::object PyRocVideoDecoder::PyGetNumOfFlushedFrames() { 
    int32_t ret = GetNumOfFlushedFrames();
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoder::PyReleaseFrame(PyPacketData& packet) {  
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
py::object PyRocVideoDecoder::PySaveRgbFrameToFile(std::string& output_file_name_in, uintptr_t& surf_mem, int width, int height, int rgb_format, uintptr_t& surf_info) {
    return PySaveTensorToFile(output_file_name_in, surf_mem, width, height, rgb_format, surf_info);
}

// for python binding
py::object PyRocVideoDecoder::PySaveTensorToFile(std::string& output_file_name_in, uintptr_t& surf_mem, int width, int height, int rgb_format, uintptr_t& in_surf_info) {
    OutputFormatEnum e_output_format = (OutputFormatEnum)rgb_format;
    if(surf_mem == 0 || width <= 0 || height <= 0 || in_surf_info == 0)
        return py::cast<py::none>(Py_None);
    OutputSurfaceInfo* surf_info = reinterpret_cast<OutputSurfaceInfo*>(in_surf_info);
    size_t rgb_image_size = CalculateRgbImageSize(e_output_format, surf_info);
    SaveFrameToFile(output_file_name_in, (void *)surf_mem, surf_info, rgb_image_size);
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
 
// for python binding
py::object PyRocVideoDecoder::PyInitMd5() {
    InitMd5();
    return py::cast<py::none>(Py_None);
}

// for python binding
py::object PyRocVideoDecoder::PyUpdateMd5ForFrame(uintptr_t& surf_mem, uintptr_t& surface_info) {  
    if(surface_info && surf_mem)
        UpdateMd5ForFrame((void *)surf_mem, reinterpret_cast<OutputSurfaceInfo*>(surface_info));
    return py::cast<py::none>(Py_None);
}

// for python binding
py::object PyRocVideoDecoder::PyFinalizeMd5(uintptr_t& digest_back) {    
    uint8_t * digest;
    FinalizeMd5(&digest);
    memcpy(reinterpret_cast<uint8_t*>(digest_back), digest,  sizeof(uint8_t) * 16);
    return py::cast<py::none>(Py_None);
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetWidth() {    
    return py::int_(static_cast<int>(GetWidth()));
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetHeight() {    
    return py::int_(static_cast<int>(GetHeight()));
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetFrameSize() {    
    return py::int_(static_cast<int>(GetFrameSize()));
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetStride() {
    return py::int_(static_cast<int>(GetSurfaceStride()));
}
