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

#include "../inc/roc_pyvideodecode.h" 

using namespace std;

using namespace pybind11::literals; // NOLINT

static void *ctypes_void_ptr(const py::object &object) {
    auto ptr_as_int = getattr(object, "value", py::none());
    if (ptr_as_int.is_none()) {
        return nullptr;
    }
    void *ptr = PyLong_AsVoidPtr(ptr_as_int.ptr());
    return ptr;
} 

void Init_pyRocVideoDecoder(py::module& m)
{
        py::class_<pyRocVideoDecoder> (m, "pyRocVideoDecoder")
        .def(py::init<int,OutputSurfaceMemoryType,rocDecVideoCodec,bool,const Rect *,bool,int,int,uint32_t>(),
                    py::arg("device_id"), py::arg("out_mem_type"), py::arg("codec"),
                    py::arg("force_zero_latency"), py::arg("p_crop_rect"), py::arg("extract_user_SEI_Message"),
                    py::arg("max_width"), py::arg("max_height"), py::arg("clk_rate"))
        .def("GetDeviceinfo",&pyRocVideoDecoder::wrapper_GetDeviceinfo)
        .def("InitMd5",&pyRocVideoDecoder::wrapper_InitMd5)
        .def("SetReconfigParams",&pyRocVideoDecoder::wrapper_SetReconfigParams)
        .def("DecodeFrame",&pyRocVideoDecoder::wrapper_DecodeFrame) 
        .def("GetFrameAddress",&pyRocVideoDecoder::wrapper_GetFrameAddress)
        .def("SaveFrameToFile",&pyRocVideoDecoder::wrapper_SaveFrameToFile)
        .def("ReleaseFrame",&pyRocVideoDecoder::wrapper_ReleaseFrame)
        .def("GetOutputSurfaceInfoAdrs",&pyRocVideoDecoder::wrapper_GetOutputSurfaceInfoAdrs)
        .def("FinalizeMd5",&pyRocVideoDecoder::wrapper_FinalizeMd5)
        .def("GetNumOfFlushedFrames",&pyRocVideoDecoder::wrapper_GetNumOfFlushedFrames)
        .def("UpdateMd5ForFrame",&pyRocVideoDecoder::wrapper_UpdateMd5ForFrame);
}


// for pyhton binding
py::object pyRocVideoDecoder::wrapper_SetReconfigParams(py::object& flush, py::object& dump,  py::object& output_file_name_in) {  
    auto p_flush = ctypes_void_ptr(flush);
    auto p_dump = ctypes_void_ptr(dump); 
    auto p_name_ptr = ctypes_void_ptr(output_file_name_in);
    int sizeofW = wcslen((wchar_t *) p_name_ptr);
    char ptr[sizeofW]; // file name + path shouldn't exceed 256
    memset(ptr,0,sizeofW);
    wcstombs(ptr,(wchar_t *) p_name_ptr, sizeofW-1); // safe copy with exact size
    std::string tmp(ptr);
    std::string output_file_name(tmp);    
    
    py_dump_file_struct.b_dump_frames_to_file = *(bool*) p_dump;
    py_dump_file_struct.output_file_name = output_file_name; 

    py_reconfig_params.reconfig_flush_mode = *(int *)p_flush;
    py_reconfig_params.p_reconfig_user_struct = (void *)((ReconfigDumpFileStruct *) &py_dump_file_struct);
    py_reconfig_params.p_fn_reconfigure_flush = (PFNRECONFIGUEFLUSHCALLBACK) &ReconfigureFlushCallback;

    SetReconfigParams((ReconfigParams*)&py_reconfig_params);   
    
    return py::cast<py::none>(Py_None);
}

int pyRocVideoDecoder::wrapper_DecodeFrame(uint64_t frame_adrs, int64_t frame_size, int pkt_flags, int64_t pts_in) {

    int ret = DecodeFrame((u_int8_t*) frame_adrs, (size_t) frame_size, pkt_flags, pts_in);
    return ret;
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_GetFrameAddress(py::array_t<int64_t>& pts_in, py::array_t<uint64_t>& frame_mem_adrs) {
   
    int64_t pts = 0;
    uint8_t* ret_frame_address = GetFrame(&pts); 

    // copy the adrs itself, not the content of the frame: Essam
    frame_mem_adrs.resize({sizeof(uint64_t)}, false);
    memcpy(frame_mem_adrs.mutable_data(), &ret_frame_address, sizeof(uint64_t));
 
    pts_in.resize({sizeof(int64_t)}, false);
    memcpy(pts_in.mutable_data(), &pts, sizeof(int64_t));
 
    return py::cast(pts);
}

// for python binding (can not move it to header for py)
py::object pyRocVideoDecoder::wrapper_GetNumOfFlushedFrames() { 
    int32_t ret = GetNumOfFlushedFrames();
    return py::cast(ret);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_ReleaseFrame(py::array_t<int64_t>& pTimestamp_in, py::array_t<bool>& b_flushing_in) {
  
    int64_t pTimestamp = 0;
    bool b_flushing = false;
    memcpy( &pTimestamp, pTimestamp_in.mutable_data(), sizeof(int64_t));
    memcpy( &b_flushing, b_flushing_in.mutable_data(), sizeof(bool));
    bool ret = ReleaseFrame(pTimestamp, b_flushing);     
    return py::cast(ret);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_SaveFrameToFile(py::object& output_file_name_in,py::array_t<uint64_t>& surf_mem_adrs, py::array_t<uint8_t>& surface_info_adrs) {
    auto p_ptr = ctypes_void_ptr(output_file_name_in);
    int sizeofW = wcslen((wchar_t *) p_ptr);
    char ptr[sizeofW+4]; // file path shouldn't exeed 256
    memset(ptr,0,sizeofW+4);
    size_t t = wcstombs(ptr,(wchar_t *) p_ptr, sizeofW); // safe copy with exact size
    std::string tmp(ptr);
    std::string output_file_name(tmp);   
    uint64_t surf_mem;
    OutputSurfaceInfo surf_info;

    memcpy(&surf_mem, surf_mem_adrs.mutable_data(), sizeof(uint64_t)); 
    memcpy(&surf_info, surface_info_adrs.mutable_data(), sizeof(OutputSurfaceInfo)); 
    
    SaveFrameToFile(output_file_name, (void *)surf_mem, &surf_info);
    
    return py::cast<py::none>(Py_None);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_InitMd5() {
    InitMd5();
    return py::cast<py::none>(Py_None);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_UpdateMd5ForFrame(py::array_t<uint64_t>& surf_mem_adrs, py::array_t<uint8_t>& surface_info_adrs) {
    
    uint64_t surf_mem;
    OutputSurfaceInfo surf_info;

    memcpy(&surf_mem, surf_mem_adrs.mutable_data(), sizeof(uint64_t)); 
    memcpy(&surf_info, surface_info_adrs.mutable_data(), sizeof(OutputSurfaceInfo));     
    UpdateMd5ForFrame((void *)surf_mem, &surf_info);
    
    return py::cast<py::none>(Py_None);
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_FinalizeMd5(py::object& digest) {    
    auto p_digest = ctypes_void_ptr(digest);
    uint8_t *to_digest;
    FinalizeMd5(&to_digest);
    memcpy(((uint8_t*)p_digest), to_digest,  sizeof(uint8_t)*16);
    return py::cast<py::none>(Py_None); 
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_GetDeviceinfo(py::object &device_name, py::object &gcn_arch_name, py::object &pci_bus_id, py::object &pci_domain_id, py::object &pci_device_id) {

    std::string l_device_name; std::string l_gcn_arch_name; int l_pci_bus_id=0; int l_pci_domain_id=0; int l_pci_device_id=0;
    GetDeviceinfo(l_device_name, l_gcn_arch_name, l_pci_bus_id, l_pci_domain_id, l_pci_device_id);

    auto ptr_device_name = ctypes_void_ptr(device_name);
    auto ptr_gcn_arch_name = ctypes_void_ptr(gcn_arch_name);
    auto ptr_pci_bus_id = ctypes_void_ptr(pci_bus_id);
    auto ptr_pci_domain_id = ctypes_void_ptr(pci_domain_id);
    auto ptr_pci_device_id = ctypes_void_ptr(pci_device_id); 
    memcpy( ptr_device_name, l_device_name.c_str(), strlen(l_device_name.c_str()));
    memcpy( ptr_gcn_arch_name, l_gcn_arch_name.c_str(), strlen(l_gcn_arch_name.c_str()));
    *(int*)ptr_pci_bus_id =  l_pci_bus_id;
    *(int*)ptr_pci_domain_id = l_pci_domain_id;
    *(int*)ptr_pci_device_id = l_pci_device_id;

    return py::cast<py::none>(Py_None); 
}

// for pyhton binding
py::object pyRocVideoDecoder::wrapper_GetOutputSurfaceInfoAdrs(OutputSurfaceInfo& surface_adrs, py::array_t<uint8_t>& surface_info_adrs) {
    
    
    OutputSurfaceInfo *l_surface_info;
    bool ret = GetOutputSurfaceInfo(&l_surface_info);

    if (ret){
        // copy whole structure in our own mem
        surface_info_adrs.resize({sizeof(OutputSurfaceInfo)}, false);
        memcpy(surface_info_adrs.mutable_data(), l_surface_info, sizeof(OutputSurfaceInfo)); 
		surface_adrs = *l_surface_info;
    }
    
    return py::cast(ret);
}

 
// callback function to flush last frames and save it to file when reconfigure happens
int ReconfigureFlushCallback(void *p_viddec_obj, uint32_t flush_mode, void *p_user_struct) {
    
    int n_frames_flushed = 0;
    if ((p_viddec_obj == nullptr) ||  (p_user_struct == nullptr)) return n_frames_flushed;

    RocVideoDecoder *viddec = static_cast<RocVideoDecoder *> (p_viddec_obj);
    OutputSurfaceInfo *surf_info;
    if (!viddec->GetOutputSurfaceInfo(&surf_info)) {
        std::cerr << "Error: Failed to get Output Surface Info!" << std::endl;
        return n_frames_flushed;
    }

    uint8_t *pframe = nullptr;
    int64_t pts;
    while ((pframe = viddec->GetFrame(&pts))) {
        if (flush_mode != RECONFIG_FLUSH_MODE_NONE) {
            if (flush_mode == ReconfigFlushMode::RECONFIG_FLUSH_MODE_DUMP_TO_FILE) {
                ReconfigDumpFileStruct *p_dump_file_struct = static_cast<ReconfigDumpFileStruct *>(p_user_struct);
                if (p_dump_file_struct->b_dump_frames_to_file) {
                    viddec->SaveFrameToFile(p_dump_file_struct->output_file_name, pframe, surf_info);
                }
            } else if (flush_mode == ReconfigFlushMode::RECONFIG_FLUSH_MODE_CALCULATE_MD5) {
                viddec->UpdateMd5ForFrame(pframe, surf_info);
            }
        }
        // release and flush frame
        viddec->ReleaseFrame(pts, true);
        n_frames_flushed ++;
    }

    return n_frames_flushed;
}

