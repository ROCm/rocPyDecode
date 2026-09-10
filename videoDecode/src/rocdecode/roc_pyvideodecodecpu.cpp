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
#include "roc_pyvideodecodecpu.h"
#include "colorspace_kernels.h"

using namespace std;

void PyRocVideoDecoderCpuInitializer(py::module& m) {
        py::class_<PyRocVideoDecoderCpu> (m, "PyRocVideoDecoderCpu")
        .def(py::init<int,int,rocDecVideoCodec,bool,const Rect *,int,int,uint32_t>(),
                    py::arg("device_id") = 0, py::arg("out_mem_type") = OUT_SURFACE_MEM_HOST_COPIED, py::arg("codec") = rocDecVideoCodec_HEVC, py::arg("force_zero_latency") = false, 
                    py::arg("p_crop_rect") = nullptr, py::arg("max_width") = 0, py::arg("max_height") = 0, py::arg("clk_rate") = 1000)
        .def("GetDeviceinfo",&PyRocVideoDecoderCpu::PyGetDeviceinfo)
        .def("DecodeFrame",&PyRocVideoDecoderCpu::PyDecodeFrame)
        .def("GetFrameYuv",&PyRocVideoDecoderCpu::PyGetFrameYuv)
        .def("GetFrameRgb",&PyRocVideoDecoderCpu::PyGetFrameRgb)
        .def("ResizeFrame",&PyRocVideoDecoderCpu::PyResizeFrame)
        .def("GetWidth",&PyRocVideoDecoderCpu::PyGetWidth)
        .def("GetHeight",&PyRocVideoDecoderCpu::PyGetHeight)
        .def("GetStride",&PyRocVideoDecoderCpu::PyGetStride)
        .def("GetFrameSize",&PyRocVideoDecoderCpu::PyGetFrameSize)
        .def("SaveFrameToFile",&PyRocVideoDecoderCpu::PySaveFrameToFile)
        .def("ReleaseFrame",&PyRocVideoDecoderCpu::PyReleaseFrame)
        .def("GetOutputSurfaceInfo",&PyRocVideoDecoderCpu::PyGetOutputSurfaceInfo)
        .def("GetResizedOutputSurfaceInfo",&PyRocVideoDecoderCpu::PyGetResizedOutputSurfaceInfo)
        .def("GetNumOfFlushedFrames",&PyRocVideoDecoderCpu::PyGetNumOfFlushedFrames)
        .def("IsCodecSupported",&PyRocVideoDecoderCpu::PyCodecSupported)
        .def("GetBitDepth",&PyRocVideoDecoderCpu::PyGetBitDepth)
#if ROCDECODE_CHECK_VERSION(0,6,0)
        .def("AddDecoderSessionOverHead",&PyRocVideoDecoderCpu::PyAddDecoderSessionOverHead)
        .def("GetDecoderSessionOverHead",&PyRocVideoDecoderCpu::PyGetDecoderSessionOverHead)
#endif
    ;
}

void PyRocVideoDecoderCpu::InitConfigStructure() {
    // init config struct
    configInfo.reset(new ConfigInfo());
    configInfo.get()->device_name = std::string("");
    configInfo.get()->gcn_arch_name = std::string("");
    configInfo.get()->pci_bus_id = 0;
    configInfo.get()->pci_domain_id = 0;
    configInfo.get()->pci_device_id = 0;
}

PyRocVideoDecoderCpu::~PyRocVideoDecoderCpu() {
    // The utility's derived and base destructors both free device frames.
    // Release them once here and leave an empty store for both destructors.
    {
        std::lock_guard<std::mutex> lock(mtx_vp_frame_);
        for (auto& frame : vp_frames_) {
            if (out_mem_type_ == OUT_SURFACE_MEM_DEV_COPIED)
                (void)hipFree(frame.frame_ptr);
            else
                delete[] frame.frame_ptr;
        }
        vp_frames_.clear();
    }
    // This is a host decoder handle; the base destructor uses the GPU API.
    if (roc_decoder_) {
        (void)rocDecDestroyDecoderHost(roc_decoder_);
        roc_decoder_ = nullptr;
    }
    rgb_owner_.reset();
    frame_ptr_rgb = nullptr;


}

void PyRocVideoDecoderCpu::ParseBitDepth(const PyPacketData& packet) {
    if (!packet.bitstream_adrs || packet.bitstream_size <= 0) return;
    if (!bit_depth_context_) {
        AVCodecID codec = AV_CODEC_ID_NONE;
        switch (codec_id_) {
        case rocDecVideoCodec_AVC: codec = AV_CODEC_ID_H264; break;
        case rocDecVideoCodec_HEVC: codec = AV_CODEC_ID_HEVC; break;
        case rocDecVideoCodec_AV1: codec = AV_CODEC_ID_AV1; break;
        case rocDecVideoCodec_VP9: codec = AV_CODEC_ID_VP9; break;
        default: return;
        }
        bit_depth_parser_ = std::shared_ptr<AVCodecParserContext>(av_parser_init(codec), av_parser_close);
        bit_depth_context_ = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(nullptr),
            [](AVCodecContext* p) { avcodec_free_context(&p); });
        if (!bit_depth_parser_ || !bit_depth_context_)
            throw std::runtime_error("Could not create the CPU bit-depth parser");
        bit_depth_context_->codec_id = codec;
        bit_depth_parser_->flags |= PARSER_FLAG_COMPLETE_FRAMES;
    }
    // The host library may report storage width (16) instead of sample depth
    // (10 or 12). Obtain the sample depth from FFmpeg's bitstream parser.
    std::vector<uint8_t> padded(packet.bitstream_size + AV_INPUT_BUFFER_PADDING_SIZE, 0);
    memcpy(padded.data(), reinterpret_cast<void*>(packet.bitstream_adrs), packet.bitstream_size);
    uint8_t* parsed = nullptr;
    int parsed_size = 0;
    const int result = av_parser_parse2(bit_depth_parser_.get(), bit_depth_context_.get(),
        &parsed, &parsed_size, padded.data(), packet.bitstream_size,
        AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (result < 0) throw std::runtime_error("Could not parse the CPU video format");
    const auto* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(bit_depth_parser_->format));
    if (desc) parsed_bit_depth_ = desc->comp[0].depth;
}

int PyRocVideoDecoderCpu::PyDecodeFrame(PyPacketData& packet) {
    ParseBitDepth(packet);
    if(packet.bitstream_size == 0)
        packet.pkt_flags |= ROCDEC_PKT_ENDOFSTREAM;
    // The nullptr argument is reserved for future use or optional parameters (e.g., get the count of images decoded).
    int decoded_frame_count = DecodeFrame(reinterpret_cast<const uint8_t *>(packet.bitstream_adrs), static_cast<size_t>(packet.bitstream_size), packet.pkt_flags, packet.frame_pts, nullptr);
    return decoded_frame_count;
}

// for python binding
py::object PyRocVideoDecoderCpu::PyGetFrameYuv(PyPacketData& packet, bool separate) {
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

size_t PyRocVideoDecoderCpu::CalculateRgbImageSize(OutputFormatEnum& e_output_format, OutputSurfaceInfo * p_surf_info) {
    const int format = static_cast<int>(e_output_format);
    if (format < 1 || format > 8)
        throw std::invalid_argument("RGB format must be in the range 1 through 8");
    const size_t channels = format >= 5 ? 4 : 3;
    const size_t item_size = format % 2 == 0 ? 2 : 1;
    return size_t((p_surf_info->output_width + 1) & ~1u) *
        p_surf_info->output_height * channels * item_size;
}

// for python binding
py::object PyRocVideoDecoderCpu::PyGetFrameRgb(PyPacketData& packet, int rgb_format) {
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
        OutputSurfaceInfo rgb_input = *info;
        if (parsed_bit_depth_) rgb_input.bit_depth = parsed_bit_depth_;
        if (rgb_input.bytes_per_pixel == 2 && !parsed_bit_depth_ && rgb_input.bit_depth == 16)
            throw std::runtime_error("CPU RGB conversion requires the actual sample bit depth");
        ConvertCpuRgbFrame(input, &rgb_input, frame_ptr_rgb, format, pitch);
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
uintptr_t PyRocVideoDecoderCpu::PyGetResizedOutputSurfaceInfo() {
    return resized_surface_.owner ? reinterpret_cast<uintptr_t>(&resized_surface_.info) : 0;
}

uintptr_t PyRocVideoDecoderCpu::PyResizeFrame(PyPacketData& packet, Dim* dim, uintptr_t& surface_info) {
    if (!dim || !surface_info || !packet.frame_adrs) return 0;
    const auto* info = reinterpret_cast<OutputSurfaceInfo*>(surface_info);
    if (dim->w == info->output_width && dim->h == info->output_height) return 0;
    HIP_API_CALL(hipSetDevice(device_id_));
    resized_surface_.Resize(reinterpret_cast<uint8_t*>(packet.frame_adrs), *info, dim->w, dim->h);
    packet.frame_adrs_resized = reinterpret_cast<uintptr_t>(resized_surface_.data());
    return reinterpret_cast<uintptr_t>(&resized_surface_.info);
}

// for python binding (can not move it to header for py)
py::object PyRocVideoDecoderCpu::PyGetNumOfFlushedFrames() {
    int32_t ret = GetNumOfFlushedFrames();
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoderCpu::PyReleaseFrame(PyPacketData& packet) {
    bool ret = ReleaseFrame(packet.frame_pts);
    return py::cast(ret);
}

// for python binding
py::object PyRocVideoDecoderCpu::PySaveFrameToFile(std::string& output_file_name_in, uintptr_t& surf_mem, uintptr_t& surface_info, OutputFormatEnum e_output_format) {
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
        OutputSurfaceInfo saved_info = *p_surf_info;
        // RGB conversion allocates on the GPU even when YUV output is on the host.
        if (image_size) saved_info.mem_type = OUT_SURFACE_MEM_DEV_COPIED;
        SaveFrameToFile(output_file_name, (void *)surf_mem, &saved_info, image_size);
    }
    return py::cast<py::none>(Py_None);
}

// for python binding
std::shared_ptr<ConfigInfo> PyRocVideoDecoderCpu::PyGetDeviceinfo() {
    GetDeviceinfo(configInfo.get()->device_name, configInfo.get()->gcn_arch_name, configInfo.get()->pci_bus_id, configInfo.get()->pci_domain_id, configInfo.get()->pci_device_id);
    return configInfo; 
}

// for python binding
uintptr_t PyRocVideoDecoderCpu::PyGetOutputSurfaceInfo() {
    OutputSurfaceInfo *l_surface_info;
    bool ret = GetPythonSurfaceInfo(&l_surface_info);
    if (ret) {
       return reinterpret_cast<std::uintptr_t>(l_surface_info);
    }
    return 0;
}

// for python binding
py::int_ PyRocVideoDecoderCpu::PyGetWidth() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_width);
    return py::int_(static_cast<int>(GetWidth()));
}

// for python binding
py::int_ PyRocVideoDecoderCpu::PyGetHeight() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_height);
    return py::int_(static_cast<int>(GetHeight()));
}

// for python binding
py::int_ PyRocVideoDecoderCpu::PyGetFrameSize() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_surface_size_in_bytes);
    return py::int_(static_cast<int>(GetFrameSize()));
}

// for python binding
py::int_ PyRocVideoDecoderCpu::PyGetStride() {
    OutputSurfaceInfo* info = nullptr;
    if (rocpy::HasCrop(requested_crop_) && GetPythonSurfaceInfo(&info)) return py::int_(info->output_pitch);
    return py::int_(static_cast<int>(GetSurfaceStride()));
}

// for python binding
py::object PyRocVideoDecoderCpu::PyCodecSupported(int device_id, rocDecVideoCodec codec_id, uint32_t bit_depth) {
    bool ret = CodecSupported(device_id, codec_id, bit_depth);
    return py::cast(ret);
}

uint32_t PyRocVideoDecoderCpu::PyGetBitDepth() {
    return parsed_bit_depth_ ? parsed_bit_depth_ : GetBitDepth();
}

#if ROCDECODE_CHECK_VERSION(0,6,0)
// for python binding, Session overhead refers to decoder initialization and deinitialization time
py::object PyRocVideoDecoderCpu::PyAddDecoderSessionOverHead(int session_id, double duration) {
    AddDecoderSessionOverHead(static_cast<std::thread::id>(session_id), duration);
    return py::cast<py::none>(Py_None);
}

// for python binding, Session overhead refers to decoder initialization and deinitialization time
py::object PyRocVideoDecoderCpu::PyGetDecoderSessionOverHead(int session_id) {
    return py::cast(GetDecoderSessionOverHead(static_cast<std::thread::id>(session_id)));
}

#endif

// Crop after native decoding so utility allocations and copies use full-frame dimensions.
bool PyRocVideoDecoderCpu::GetPythonSurfaceInfo(OutputSurfaceInfo** info) {
    if (!FFMpegVideoDecoder::GetOutputSurfaceInfo(info)) return false;
    if (rocpy::HasCrop(requested_crop_)) {
        cropped_surface_.info = rocpy::Describe(**info,
            requested_crop_.right - requested_crop_.left, requested_crop_.bottom - requested_crop_.top,
            (*info)->mem_type == OUT_SURFACE_MEM_HOST_COPIED);
        *info = &cropped_surface_.info;
    }
    return true;
}

uint8_t* PyRocVideoDecoderCpu::GetPythonFrame(int64_t* pts) {
    auto input = FFMpegVideoDecoder::GetFrame(pts);
    if (!input || !rocpy::HasCrop(requested_crop_)) return input;
    try {
        OutputSurfaceInfo* full = nullptr;
        if (!FFMpegVideoDecoder::GetOutputSurfaceInfo(&full)) throw std::runtime_error("Missing output surface information");
        if (full->mem_type != OUT_SURFACE_MEM_HOST_COPIED) HIP_API_CALL(hipSetDevice(device_id_));
        cropped_surface_.Copy(input, *full, requested_crop_, full->mem_type == OUT_SURFACE_MEM_HOST_COPIED);
        return cropped_surface_.data();
    } catch (...) {
        ReleaseFrame(*pts);
        throw;
    }
}
