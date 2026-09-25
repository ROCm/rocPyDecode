/* Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#include "roc_pydecode.h"
#include "roc_pysurface.h"
#include "roc_pyrgb.h"
#include <fstream>

namespace {
// Own decoded planes and provide crop, resize, RGB and DLPack operations.
// Compressed-video decoding is handled by the caller.
class CpuSurface {
    rocpy::Surface surface_, resized_;
    std::shared_ptr<void> rgb_;
    size_t rgb_size_ = 0;
    int device_, memory_;

    void Export(PyPacketData& packet, void* data, std::shared_ptr<void> owner,
                std::vector<size_t> shape, std::vector<size_t> strides,
                uint32_t bytes, DLDeviceType device, size_t index = 0) {
        auto buffer = std::make_shared<BufferInterface>();
        std::string type = bytes == 1 ? "|u1" : "|u2";
        buffer->KeepAlive(std::move(owner));
        buffer->LoadDLPack(shape, strides, bytes * 8, type, data, device_, device);
        packet.ext_buf.at(index) = std::move(buffer);
    }
public:
    CpuSurface(const std::vector<py::buffer>& planes, const std::vector<size_t>& pitches,
               int width, int height, uint32_t depth, rocDecVideoSurfaceFormat format,
               int device, int memory, const Rect& crop) : device_(device), memory_(memory) {
        if (device < 0 || (memory != OUT_SURFACE_MEM_HOST_COPIED && memory != OUT_SURFACE_MEM_DEV_COPIED))
            throw std::invalid_argument("Invalid CPU frame device or memory type");
        Update(planes, pitches, width, height, depth, format, crop);
    }

    void Update(const std::vector<py::buffer>& planes, const std::vector<size_t>& pitches,
                int width, int height, uint32_t depth, rocDecVideoSurfaceFormat format,
                const Rect& crop) {
        if (depth < 8 || depth > 16)
            throw std::invalid_argument("Invalid CPU frame bit depth");
        OutputSurfaceInfo source{};
        source.surface_format = format;
        source.bytes_per_pixel = depth > 8 ? 2 : 1;
        source.bit_depth = depth;
        source.num_chroma_planes = 2;
        source = rocpy::Describe(source, width, height, true);
        auto layout = rocpy::Planes(source);
        if (planes.size() != layout.count || pitches.size() != layout.count)
            throw std::invalid_argument("CPU frame has an unexpected number of planes");
        // PyAV planes may have independent padded pitches; pack each row.
        std::vector<uint8_t> packed(source.output_surface_size_in_bytes);
        for (size_t i = 0; i < layout.count; ++i) {
            auto input = planes[i].request();
            const auto& plane = layout.planes[i];
            const size_t row = size_t(plane.width) * plane.channels * source.bytes_per_pixel;
            if (input.ndim != 1 || input.itemsize != 1 || input.strides[0] != 1 ||
                pitches[i] < row || pitches[i] > size_t(input.size) / plane.height)
                throw std::invalid_argument("CPU plane buffer is too small or non-contiguous");
            for (size_t y = 0; y < plane.height; ++y)
                memcpy(packed.data() + plane.offset + y * plane.pitch,
                       static_cast<uint8_t*>(input.ptr) + y * pitches[i], row);
        }
        const bool host = memory_ == OUT_SURFACE_MEM_HOST_COPIED;
        if (!host) HIP_API_CALL(hipSetDevice(device_));
        surface_.Copy(packed.data(), source, rocpy::HasCrop(crop) ? crop : Rect{0, 0, width, height}, host);
        resized_ = rocpy::Surface{};
    }

    void Yuv(PyPacketData& packet, bool separate) {
        auto& info = surface_.info;
        auto layout = rocpy::Planes(info);
        packet.frame_adrs = reinterpret_cast<uintptr_t>(surface_.data());
        packet.frame_size = static_cast<int64_t>(info.output_surface_size_in_bytes);
        const auto device = info.mem_type == OUT_SURFACE_MEM_HOST_COPIED ? kDLCPU : kDLROCM;
        for (size_t i = 0; i < (separate ? layout.count : 1); ++i) {
            const auto& plane = layout.planes[i];
            // Storage contains consecutive Y, U and V samples without padding.
            // The combined view groups these samples into luma-width rows;
            // two subsampled chroma rows occupy one such row.
            const size_t rows = separate ? plane.height : layout.size / info.output_pitch;
            Export(packet, surface_.data() + plane.offset, surface_.owner,
                   {rows, size_t(plane.width) * plane.channels},
                   {plane.pitch, info.bytes_per_pixel}, info.bytes_per_pixel, device, i);
        }
    }

    void Rgb(PyPacketData& packet, int output_format) {
        if (output_format < 1 || output_format > 8)
            throw std::invalid_argument("RGB format must be in the range 1 through 8");
        HIP_API_CALL(hipSetDevice(device_));
        auto& info = surface_.info;
        auto format = static_cast<OutputFormatEnum>(output_format);
        const uint32_t pitch = CalculateRgbPitch(info.output_width, format);
        const size_t size = size_t(pitch) * info.output_height;
        if (!rgb_ || rgb_.use_count() > 1 || rgb_size_ != size) {
            void* allocation = nullptr;
            HIP_API_CALL(hipMalloc(&allocation, size));
            rgb_ = std::shared_ptr<void>(allocation, [](void* p) { (void)hipFree(p); });
            rgb_size_ = size;
        }
        void* data = rgb_.get();
        ConvertCpuRgbFrame(surface_.data(), &info, static_cast<uint8_t*>(data), format, pitch);
        const size_t channels = output_format >= 5 ? 4 : 3;
        const uint32_t bytes = output_format % 2 == 0 ? 2 : 1;
        packet.frame_adrs = reinterpret_cast<uintptr_t>(surface_.data());
        packet.frame_adrs_rgb = reinterpret_cast<uintptr_t>(data);
        Export(packet, data, rgb_, {info.output_height, info.output_width, channels},
               {pitch, channels * bytes, bytes}, bytes, kDLROCM);
    }

    uintptr_t Resize(PyPacketData& packet, const Dim& dim, uintptr_t info) {
        if (info != InfoAddress() || packet.frame_adrs != reinterpret_cast<uintptr_t>(surface_.data()))
            throw std::invalid_argument("Resize requires this decoder's current CPU frame");
        if (int64_t(dim.w) == surface_.info.output_width && int64_t(dim.h) == surface_.info.output_height)
            return 0;
        HIP_API_CALL(hipSetDevice(device_));
        resized_.Resize(surface_.data(), surface_.info, dim.w, dim.h);
        packet.frame_adrs_resized = reinterpret_cast<uintptr_t>(resized_.data());
        return ResizedInfoAddress();
    }

    void Save(const std::string& path, uintptr_t address, uintptr_t info, OutputFormatEnum format) {
        bool host = false;
        size_t size = 0;
        if (format != OutputFormatEnum::native) {
            if (!rgb_ || address != reinterpret_cast<uintptr_t>(rgb_.get()))
                throw std::invalid_argument("Unknown CPU RGB buffer");
            size = rgb_size_;
        } else if (address == reinterpret_cast<uintptr_t>(surface_.data()) && (!info || info == InfoAddress())) {
            size = surface_.info.output_surface_size_in_bytes;
            host = surface_.info.mem_type == OUT_SURFACE_MEM_HOST_COPIED;
        } else if (resized_.owner && address == reinterpret_cast<uintptr_t>(resized_.data()) && info == ResizedInfoAddress()) {
            size = resized_.info.output_surface_size_in_bytes;
        } else {
            throw std::invalid_argument("Unknown CPU frame buffer or surface information");
        }
        auto data = reinterpret_cast<const uint8_t*>(address);
        std::vector<uint8_t> copy;
        if (!host) {
            HIP_API_CALL(hipSetDevice(device_));
            copy.resize(size);
            HIP_API_CALL(hipMemcpy(copy.data(), data, size, hipMemcpyDeviceToHost));
            data = copy.data();
        }
        std::ofstream output(path, std::ios::binary | std::ios::app);
        if (!output || !output.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size)))
            throw std::runtime_error("Could not write CPU frame to " + path);
    }

    uintptr_t InfoAddress() { return reinterpret_cast<uintptr_t>(&surface_.info); }
    uintptr_t ResizedInfoAddress() { return resized_.owner ? reinterpret_cast<uintptr_t>(&resized_.info) : 0; }
    uint32_t Width() const { return surface_.info.output_width; }
    uint32_t Height() const { return surface_.info.output_height; }
    uint32_t Pitch() const { return surface_.info.output_pitch; }
    size_t Size() const { return surface_.info.output_surface_size_in_bytes; }
};
}

void PyCpuSurfaceInitializer(py::module& m) {
    m.def("_cpu_device_info", [](int device) {
        hipDeviceProp_t properties{};
        HIP_API_CALL(hipGetDeviceProperties(&properties, device));
        auto info = std::make_shared<ConfigInfo>();
        info->device_name = properties.name;
        info->gcn_arch_name = properties.gcnArchName;
        info->pci_bus_id = properties.pciBusID;
        info->pci_domain_id = properties.pciDomainID;
        info->pci_device_id = properties.pciDeviceID;
        return info;
    });
    py::class_<CpuSurface>(m, "_CpuSurface")
        .def(py::init<const std::vector<py::buffer>&, const std::vector<size_t>&, int, int,
             uint32_t, rocDecVideoSurfaceFormat, int, int, const Rect&>())
        .def("Yuv", &CpuSurface::Yuv).def("Rgb", &CpuSurface::Rgb)
        .def("Update", &CpuSurface::Update)
        .def("Resize", &CpuSurface::Resize).def("Save", &CpuSurface::Save)
        .def_property_readonly("info_address", &CpuSurface::InfoAddress)
        .def_property_readonly("resized_info_address", &CpuSurface::ResizedInfoAddress)
        .def_property_readonly("width", &CpuSurface::Width)
        .def_property_readonly("height", &CpuSurface::Height)
        .def_property_readonly("pitch", &CpuSurface::Pitch)
        .def_property_readonly("size", &CpuSurface::Size);
}
