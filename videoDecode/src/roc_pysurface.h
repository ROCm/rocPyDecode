// Copyright © Advanced Micro Devices, Inc., or its affiliates.
// SPDX-License-Identifier: MIT
#pragma once

#include "roc_video_dec.h"
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>

namespace rocpy {
struct Plane {
    size_t offset, pitch;
    uint32_t width, height, channels;
};
struct Layout {
    std::array<Plane, 3> planes{};
    int count = 1, x_shift = 0, y_shift = 0;
    size_t size = 0;
};
inline Layout Planes(const OutputSurfaceInfo& info) {
    Layout result;
    bool interleaved = false;
    switch (info.surface_format) {
    case rocDecVideoSurfaceFormat_NV12:
    case rocDecVideoSurfaceFormat_P016:
        interleaved = true; result.x_shift = result.y_shift = 1; break;
    case rocDecVideoSurfaceFormat_YUV420:
    case rocDecVideoSurfaceFormat_YUV420_16Bit:
        result.x_shift = result.y_shift = 1; break;
    case rocDecVideoSurfaceFormat_YUV422:
    case rocDecVideoSurfaceFormat_YUV422_16Bit: result.x_shift = 1; break;
    case rocDecVideoSurfaceFormat_YUV444:
    case rocDecVideoSurfaceFormat_YUV444_16Bit: break;
    default: throw std::invalid_argument("Unsupported YUV surface format");
    }
    result.planes[0] = {0, info.output_pitch, info.output_width, info.output_height, 1};
    result.size = size_t(info.output_pitch) * info.output_vstride;
    if (info.num_chroma_planes) {
        result.count = interleaved ? 2 : 3;
        const size_t pitch = interleaved ? info.output_pitch : info.output_pitch >> result.x_shift;
        for (int i = 1; i < result.count; ++i) {
            result.planes[i] = {result.size, pitch, info.output_width >> result.x_shift,
                info.output_height >> result.y_shift, interleaved ? 2u : 1u};
            result.size += pitch * (info.output_vstride >> result.y_shift);
        }
    }
    return result;
}

inline bool HasCrop(const Rect& rect) {
    return rect.left || rect.top || rect.right || rect.bottom;
}
inline Rect CheckCrop(Rect rect) {
    if (HasCrop(rect) && (rect.left < 0 || rect.top < 0 || rect.right <= rect.left || rect.bottom <= rect.top))
        throw std::invalid_argument("Crop must have nonnegative coordinates and positive width and height");
    return rect;
}
// Validate before the native decoder constructor allocates resources.
inline const Rect* DecodeFullFrame(const Rect* crop) {
    CheckCrop(crop ? *crop : Rect{});
    return nullptr;
}
inline OutputSurfaceInfo Describe(const OutputSurfaceInfo& source, int width, int height, bool host) {
    const auto layout = Planes(source);
    if (width <= 0 || height <= 0 || (width % (1 << layout.x_shift)) || (height % (1 << layout.y_shift)))
        throw std::invalid_argument("YUV dimensions must be positive and aligned to chroma subsampling");
    if (source.bytes_per_pixel != 1 && source.bytes_per_pixel != 2)
        throw std::invalid_argument("Unsupported YUV element size");
    if (size_t(width) * source.bytes_per_pixel > std::numeric_limits<uint32_t>::max())
        throw std::invalid_argument("YUV row size is too large");
    auto info = source;
    info.output_width = width; info.output_height = height;
    info.output_pitch = width * source.bytes_per_pixel;
    info.output_vstride = height;
    info.chroma_height = height >> layout.y_shift;
    info.disp_rect = {0, 0, width, height};
    info.mem_type = host ? OUT_SURFACE_MEM_HOST_COPIED : OUT_SURFACE_MEM_DEV_COPIED;
    info.output_surface_size_in_bytes = Planes(info).size;
    return info;
}

// Center-sampled nearest-neighbor resize, preserving the source sample depth.
template<typename T>
__global__ static void ResizePlane(const uint8_t* input, size_t in_pitch, uint32_t in_width,
                                  uint32_t in_height, uint8_t* output, size_t out_pitch,
                                  uint32_t width, uint32_t height, uint32_t channels) {
    const uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    const size_t sx = (uint64_t(2) * x + 1) * in_width / (uint64_t(2) * width);
    const size_t sy = (uint64_t(2) * y + 1) * in_height / (uint64_t(2) * height);
    const T* src = reinterpret_cast<const T*>(input + sy * in_pitch) + sx * channels;
    T* dst = reinterpret_cast<T*>(output + size_t(y) * out_pitch) + size_t(x) * channels;
    for (uint32_t c = 0; c < channels; ++c) dst[c] = src[c];
}

class Surface {
public:
    OutputSurfaceInfo info{};
    std::shared_ptr<void> owner;
    uint8_t* data() const { return static_cast<uint8_t*>(owner.get()); }

    void Allocate(const OutputSurfaceInfo& description) {
        if (!owner || owner.use_count() > 1 || capacity_ != description.output_surface_size_in_bytes ||
            info.mem_type != description.mem_type) {
            void* ptr = nullptr;
            if (description.mem_type == OUT_SURFACE_MEM_HOST_COPIED) {
                ptr = new uint8_t[description.output_surface_size_in_bytes];
                owner = std::shared_ptr<void>(ptr, [](void* p) { delete[] static_cast<uint8_t*>(p); });
            } else {
                HIP_API_CALL(hipMalloc(&ptr, description.output_surface_size_in_bytes));
                owner = std::shared_ptr<void>(ptr, [](void* p) { (void)hipFree(p); });
            }
            capacity_ = description.output_surface_size_in_bytes;
        }
        // Different dimensions can share an allocation size; always refresh metadata.
        info = description;
    }

    void Copy(uint8_t* input, const OutputSurfaceInfo& source, Rect rect, bool host) {
        CheckCrop(rect);
        const auto src = Planes(source);
        if (rect.right > source.output_width || rect.bottom > source.output_height ||
            rect.left % (1 << src.x_shift) || rect.top % (1 << src.y_shift))
            throw std::invalid_argument("Crop must lie within the frame and align to chroma subsampling");
        Allocate(Describe(source, rect.right - rect.left, rect.bottom - rect.top, host));
        const auto dst = Planes(info);
        const bool src_host = source.mem_type == OUT_SURFACE_MEM_HOST_COPIED;
        const auto kind = src_host ? (host ? hipMemcpyHostToHost : hipMemcpyHostToDevice) :
                                    (host ? hipMemcpyDeviceToHost : hipMemcpyDeviceToDevice);
        for (int i = 0; i < src.count; ++i) {
            const int left = i ? rect.left >> src.x_shift : rect.left;
            const int top = i ? rect.top >> src.y_shift : rect.top;
            const auto& s = src.planes[i]; const auto& d = dst.planes[i];
            const auto* ptr = input + s.offset + size_t(top) * s.pitch +
                size_t(left) * s.channels * source.bytes_per_pixel;
            const size_t row_bytes = size_t(d.width) * d.channels * source.bytes_per_pixel;
            if (src_host && host) {
                for (uint32_t row = 0; row < d.height; ++row)
                    memcpy(data() + d.offset + row * d.pitch, ptr + row * s.pitch, row_bytes);
            } else {
                HIP_API_CALL(hipMemcpy2D(data() + d.offset, d.pitch, ptr, s.pitch, row_bytes, d.height, kind));
            }
        }
    }

    void Resize(uint8_t* input, const OutputSurfaceInfo& source, int width, int height) {
        const auto description = Describe(source, width, height, false);
        Surface staging;
        const OutputSurfaceInfo* in_info = &source;
        if (source.mem_type == OUT_SURFACE_MEM_HOST_COPIED) {
            staging.Copy(input, source, {0, 0, int(source.output_width), int(source.output_height)}, false);
            input = staging.data(); in_info = &staging.info;
        }
        Allocate(description);
        const auto src = Planes(*in_info), dst = Planes(info);
        for (int i = 0; i < src.count; ++i) {
            const auto& s = src.planes[i]; const auto& d = dst.planes[i];
            const dim3 block(16,16), grid((d.width + 15)/16, (d.height + 15)/16);
            if (source.bytes_per_pixel == 1)
                ResizePlane<uint8_t><<<grid,block>>>(input+s.offset,s.pitch,s.width,s.height,
                    data()+d.offset,d.pitch,d.width,d.height,d.channels);
            else
                ResizePlane<uint16_t><<<grid,block>>>(input+s.offset,s.pitch,s.width,s.height,
                    data()+d.offset,d.pitch,d.width,d.height,d.channels);
            HIP_API_CALL(hipGetLastError());
        }
        // Input/staging storage may be released immediately after this API returns.
        HIP_API_CALL(hipStreamSynchronize(0));
    }
private:
    size_t capacity_ = 0;
};
} // namespace rocpy
