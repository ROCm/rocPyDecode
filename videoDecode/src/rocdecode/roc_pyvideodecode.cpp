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

#include "roc_pyrgb.h"
#include "roc_pyvideodecode.h"
#include "colorspace_kernels.h"

using namespace std;

void PyRocVideoDecoderInitializer(py::module& m) {
        py::class_<PyRocVideoDecoder> (m, "PyRocVideoDecoder")
        .def(py::init<int,int,rocDecVideoCodec,bool,const Rect *,int,int,uint32_t>(),
                    py::arg("device_id") = 0, py::arg("out_mem_type") = OUT_SURFACE_MEM_DEV_INTERNAL, py::arg("codec") = rocDecVideoCodec_HEVC, py::arg("force_zero_latency") = false, 
                    py::arg("p_crop_rect") = nullptr, py::arg("max_width") = 0, py::arg("max_height") = 0, py::arg("clk_rate") = 1000)
        .def("GetDeviceinfo",&PyRocVideoDecoder::PyGetDeviceinfo)
        .def("DecodeFrame",&PyRocVideoDecoder::PyDecodeFrame)
        .def("GetFrameYuv",&PyRocVideoDecoder::PyGetFrameYuv)
        .def("GetFrameRgb",&PyRocVideoDecoder::PyGetFrameRgb)
        .def("ResizeFrame",&PyRocVideoDecoder::PyResizeFrame)
        .def("GetWidth",&PyRocVideoDecoder::PyGetWidth)
        .def("GetHeight",&PyRocVideoDecoder::PyGetHeight)
        .def("GetStride",&PyRocVideoDecoder::PyGetStride)
        .def("GetFrameSize",&PyRocVideoDecoder::PyGetFrameSize)
        .def("SaveFrameToFile",&PyRocVideoDecoder::PySaveFrameToFile)
        .def("ReleaseFrame",&PyRocVideoDecoder::PyReleaseFrame)
        .def("GetOutputSurfaceInfo",&PyRocVideoDecoder::PyGetOutputSurfaceInfo)
        .def("GetResizedOutputSurfaceInfo",&PyRocVideoDecoder::PyGetResizedOutputSurfaceInfo)
        .def("GetNumOfFlushedFrames",&PyRocVideoDecoder::PyGetNumOfFlushedFrames)
        .def("SetReconfigParams",&PyRocVideoDecoder::PySetReconfigParams)
        .def("IsCodecSupported",&PyRocVideoDecoder::PyCodecSupported)
        .def("GetBitDepth",&PyRocVideoDecoder::PyGetBitDepth)
#if ROCDECODE_CHECK_VERSION(0,6,0)
        .def("AddDecoderSessionOverHead",&PyRocVideoDecoder::PyAddDecoderSessionOverHead)
        .def("GetDecoderSessionOverHead",&PyRocVideoDecoder::PyGetDecoderSessionOverHead)
#endif
    ;
}

// callback function to flush last frames and save it to file when reconfigure happens
// reference to non-static member function must be called
int PyReconfigureFlushCallback(void *p_viddec_obj, uint32_t flush_mode, void * p_user_struct) {
    int n_frames_flushed = 0;
    if ((p_viddec_obj == nullptr) ||  (p_user_struct == nullptr))
        return n_frames_flushed;
    auto* viddec = static_cast<PyRocVideoDecoder*>(static_cast<RocVideoDecoder*>(p_viddec_obj));
    OutputSurfaceInfo *surf_info;
    if (!viddec->GetPythonSurfaceInfo(&surf_info)) {
        std::cerr << "Error: Failed to get Output Surface Info!" << std::endl;
        return n_frames_flushed;
    }
    uint8_t *pframe = nullptr;
    int64_t pts;
    while ((pframe = viddec->GetPythonFrame(&pts))) {
        if (flush_mode != RECONFIG_FLUSH_MODE_NONE) {
            if (flush_mode == ReconfigFlushMode::RECONFIG_FLUSH_MODE_DUMP_TO_FILE) {
                ReconfigDumpFileStruct *p_dump_file_struct = static_cast<ReconfigDumpFileStruct *>(p_user_struct);
                if (p_dump_file_struct->b_dump_frames_to_file) {
                    viddec->SaveFrameToFile(p_dump_file_struct->output_file_name, pframe, surf_info);
                }
            }
        }
        // release and flush frame
        // ReconfigureDecoder frees copied frames after this callback returns.
        viddec->ReleaseFrame(pts);
        n_frames_flushed ++;
    }
    return n_frames_flushed;
}

py::object PyRocVideoDecoder::PySetReconfigParams(int flush_mode, std::string& output_file_name_in) {
    ReconfigFlushMode mode = static_cast<ReconfigFlushMode>(flush_mode);
    if(!output_file_name_in.empty()) {
        PyReconfigDumpFileStruct.output_file_name = output_file_name_in;
        PyReconfigDumpFileStruct.b_dump_frames_to_file = true;
    } else {
        if(mode == RECONFIG_FLUSH_MODE_DUMP_TO_FILE)
            mode = RECONFIG_FLUSH_MODE_NONE;
    }
    PyReconfigParams.p_fn_reconfigure_flush = PyReconfigureFlushCallback;
    PyReconfigParams.p_reconfig_user_struct = &PyReconfigDumpFileStruct;
    PyReconfigParams.reconfig_flush_mode = mode;
    // set the parent class
    SetReconfigParams(&PyReconfigParams);
    return py::cast<py::none>(Py_None);
}

void PyRocVideoDecoder::InitConfigStructure() {
    // init config struct
    configInfo.reset(new ConfigInfo());
    configInfo.get()->device_name = std::string("");
    configInfo.get()->gcn_arch_name = std::string("");
    configInfo.get()->pci_bus_id = 0;
    configInfo.get()->pci_domain_id = 0;
    configInfo.get()->pci_device_id = 0;
    // init flush callback struct: support multi-resolution video streams
    PyReconfigDumpFileStruct.b_dump_frames_to_file = false;
    PyReconfigDumpFileStruct.output_file_name.clear();
    PyReconfigParams.p_fn_reconfigure_flush = nullptr;
    PyReconfigParams.p_reconfig_user_struct = nullptr;
    PyReconfigParams.reconfig_flush_mode = 0;
}

PyRocVideoDecoder::~PyRocVideoDecoder() {
    rgb_owner_.reset();
    frame_ptr_rgb = nullptr;


}

int PyRocVideoDecoder::PyDecodeFrame(PyPacketData& packet) {
    if(packet.bitstream_size == 0)
        packet.pkt_flags |= ROCDEC_PKT_ENDOFSTREAM;
    int decoded_frame_count = DecodeFrame(reinterpret_cast<const uint8_t *>(packet.bitstream_adrs), static_cast<size_t>(packet.bitstream_size), packet.pkt_flags, packet.frame_pts);
    return decoded_frame_count;
}

// for python binding
py::object PyRocVideoDecoder::PyGetFrameYuv(PyPacketData& packet, bool separate) {
    int64_t pts = packet.frame_pts;
    auto input = GetPythonFrame(&pts);
    packet.frame_adrs = reinterpret_cast<uintptr_t>(input);
    packet.frame_pts = pts;
    if (!input) return py::cast(-1);
    OutputSurfaceInfo* info = nullptr;
    GetPythonSurfaceInfo(&info);
    const auto layout = rocpy::Planes(*info);
    std::string type = info->bytes_per_pixel == 1 ? "|u1" : "|u2";
    const auto device = info->mem_type == OUT_SURFACE_MEM_HOST_COPIED ? kDLCPU : kDLROCM;
    size_t rows = info->output_height;
    if (!separate)
        for (int i = 1; i < layout.count; ++i)
            rows += size_t(layout.planes[i].width) * layout.planes[i].channels * layout.planes[i].height / info->output_width;
    for (int i = 0; i < (separate ? layout.count : 1); ++i) {
        const auto& plane = layout.planes[i];
        std::vector<size_t> shape{i == 0 ? rows : plane.height, size_t(plane.width) * plane.channels};
        std::vector<size_t> strides{plane.pitch, info->bytes_per_pixel};
        auto buffer = std::make_shared<BufferInterface>();
        if (rocpy::HasCrop(requested_crop_)) buffer->KeepAlive(cropped_surface_.owner);
        buffer->LoadDLPack(shape, strides, info->bytes_per_pixel * 8, type,
            input + plane.offset, device_id_, device);
        packet.ext_buf[i] = std::move(buffer);
    }
    return py::cast(pts);
}

size_t PyRocVideoDecoder::CalculateRgbImageSize(OutputFormatEnum& e_output_format, OutputSurfaceInfo * p_surf_info) {
    const int format = static_cast<int>(e_output_format);
    if (format < 1 || format > 8)
        throw std::invalid_argument("RGB format must be in the range 1 through 8");
    const size_t channels = format >= 5 ? 4 : 3;
    const size_t item_size = format % 2 == 0 ? 2 : 1;
    return size_t((p_surf_info->output_width + 1) & ~1u) *
        p_surf_info->output_height * channels * item_size;
}

// for python binding
py::object PyRocVideoDecoder::PyGetFrameRgb(PyPacketData& packet, int rgb_format) {
    if (rgb_format < 1 || rgb_format > 8)
        throw std::invalid_argument("RGB format must be in the range 1 through 8");
    const auto format = static_cast<OutputFormatEnum>(rgb_format);
    const uint32_t channels = rgb_format >= 5 ? 4 : 3;
    const uint32_t item_size = rgb_format % 2 == 0 ? 2 : 1;
    int64_t pts = packet.frame_pts;
    auto input = GetPythonFrame(&pts);
    packet.frame_adrs = reinterpret_cast<std::uintptr_t>(input);
    packet.frame_pts = pts;
    if (!input) return py::cast(-1);
    try {
        OutputSurfaceInfo* info = nullptr;
        GetPythonSurfaceInfo(&info);
        if (!info) throw std::runtime_error("Missing output surface information");
        HIP_API_CALL(hipSetDevice(device_id_));
        const uint32_t pitch = ((info->output_width + 1) & ~1u) * channels * item_size;
        const size_t size = size_t(pitch) * info->output_height;
        if (!rgb_owner_ || rgb_owner_.use_count() > 1 || rgb_capacity_ != size) {
            void* allocation = nullptr;
            HIP_API_CALL(hipMalloc(&allocation, size));
            rgb_owner_ = std::shared_ptr<void>(allocation, [](void* p) { (void)hipFree(p); });
            frame_ptr_rgb = static_cast<uint8_t*>(allocation);
            rgb_capacity_ = size;
        }
        ConvertRgbFrame(input, info, frame_ptr_rgb, format, pitch);
        HIP_API_CALL(hipStreamSynchronize(0));
        packet.frame_adrs_rgb = reinterpret_cast<std::uintptr_t>(frame_ptr_rgb);
        std::vector<size_t> shape{info->output_height, info->output_width, channels};
        std::vector<size_t> stride{pitch, channels * item_size, item_size};
        std::string type = item_size == 1 ? "|u1" : "|u2";
        auto buffer = std::make_shared<BufferInterface>();
        buffer->KeepAlive(rgb_owner_);
        buffer->LoadDLPack(shape, stride, item_size * 8, type, frame_ptr_rgb, device_id_);
        packet.ext_buf[0] = std::move(buffer);
    } catch (...) {
        ReleaseFrame(pts);
        throw;
    }
    return py::cast(pts);
}

// for python binding
uintptr_t PyRocVideoDecoder::PyGetResizedOutputSurfaceInfo() {
    return resized_surface_.owner ? reinterpret_cast<uintptr_t>(&resized_surface_.info) : 0;
}

uintptr_t PyRocVideoDecoder::PyResizeFrame(PyPacketData& packet, Dim* dim, uintptr_t& surface_info) {
    if (!dim || !surface_info || !packet.frame_adrs) return 0;
    const auto* info = reinterpret_cast<OutputSurfaceInfo*>(surface_info);
    if (dim->w == info->output_width && dim->h == info->output_height) return 0;
    HIP_API_CALL(hipSetDevice(device_id_));
    resized_surface_.Resize(reinterpret_cast<uint8_t*>(packet.frame_adrs), *info, dim->w, dim->h);
    packet.frame_adrs_resized = reinterpret_cast<uintptr_t>(resized_surface_.data());
    return reinterpret_cast<uintptr_t>(&resized_surface_.info);
}

// for python binding (can not move it to header for py)
py::object PyRocVideoDecoder::PyGetNumOfFlushedFrames() {
    int32_t ret = GetNumOfFlushedFrames();
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoder::PyReleaseFrame(PyPacketData& packet) {
    bool ret = ReleaseFrame(packet.frame_pts);
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoder::PySaveFrameToFile(std::string& output_file_name_in, uintptr_t& surf_mem, uintptr_t& surface_info, OutputFormatEnum e_output_format) {
    std::string output_file_name = output_file_name_in.c_str();
    OutputSurfaceInfo *p_surf_info;
    bool ret = true;
    if (surface_info)
        p_surf_info = reinterpret_cast<OutputSurfaceInfo*>(surface_info);
    else
        ret = GetPythonSurfaceInfo(&p_surf_info);
    if(surf_mem && ret) {
        size_t image_size = 0; // 0 size == rgb frame
        if (e_output_format != OutputFormatEnum::native) { // native == YUV frame
            image_size = CalculateRgbImageSize(e_output_format, p_surf_info);
        }
        auto saved_info = *p_surf_info;
        if (image_size) saved_info.mem_type = OUT_SURFACE_MEM_DEV_COPIED;
        SaveFrameToFile(output_file_name, (void *)surf_mem, &saved_info, image_size);
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
    bool ret = GetPythonSurfaceInfo(&l_surface_info);
    if (ret) {
       return reinterpret_cast<std::uintptr_t>(l_surface_info);
    }
    return 0;
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetWidth() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_width);
    return py::int_(static_cast<int>(GetWidth()));
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetHeight() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_height);
    return py::int_(static_cast<int>(GetHeight()));
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetFrameSize() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_surface_size_in_bytes);
    return py::int_(static_cast<int>(GetFrameSize()));
}

// for python binding
py::int_ PyRocVideoDecoder::PyGetStride() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_pitch);
    return py::int_(static_cast<int>(GetSurfaceStride()));
}

// for python binding
py::object PyRocVideoDecoder::PyCodecSupported(int device_id, rocDecVideoCodec codec_id, uint32_t bit_depth) {
    bool ret = CodecSupported(device_id, codec_id, bit_depth);
    return py::cast(ret);
}

uint32_t PyRocVideoDecoder::PyGetBitDepth() {
    return GetBitDepth();
}

#if ROCDECODE_CHECK_VERSION(0,6,0)
// for python binding, Session overhead refers to decoder initialization and deinitialization time
py::object PyRocVideoDecoder::PyAddDecoderSessionOverHead(int session_id, double duration) {
    AddDecoderSessionOverHead(static_cast<std::thread::id>(session_id), duration);
    return py::cast<py::none>(Py_None);
}

// for python binding, Session overhead refers to decoder initialization and deinitialization time
py::object PyRocVideoDecoder::PyGetDecoderSessionOverHead(int session_id) {
    return py::cast(GetDecoderSessionOverHead(static_cast<std::thread::id>(session_id)));
}

#endif

// Crop after native decoding so utility allocations and copies use full-frame dimensions.
bool PyRocVideoDecoder::GetPythonSurfaceInfo(OutputSurfaceInfo** info) {
    if (!RocVideoDecoder::GetOutputSurfaceInfo(info)) return false;
    if (rocpy::HasCrop(requested_crop_)) {
        cropped_surface_.info = rocpy::Describe(**info,
            requested_crop_.right - requested_crop_.left, requested_crop_.bottom - requested_crop_.top,
            (*info)->mem_type == OUT_SURFACE_MEM_HOST_COPIED);
        *info = &cropped_surface_.info;
    }
    return true;
}

uint8_t* PyRocVideoDecoder::GetPythonFrame(int64_t* pts) {
    auto input = RocVideoDecoder::GetFrame(pts);
    if (!input || !rocpy::HasCrop(requested_crop_)) return input;
    try {
        OutputSurfaceInfo* full = nullptr;
        if (!RocVideoDecoder::GetOutputSurfaceInfo(&full)) throw std::runtime_error("Missing output surface information");
        if (full->mem_type != OUT_SURFACE_MEM_HOST_COPIED) HIP_API_CALL(hipSetDevice(device_id_));
        cropped_surface_.Copy(input, *full, requested_crop_, full->mem_type == OUT_SURFACE_MEM_HOST_COPIED);
        return cropped_surface_.data();
    } catch (...) {
        ReleaseFrame(*pts);
        throw;
    }
}
