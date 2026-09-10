// Copyright © Advanced Micro Devices, Inc., or its affiliates.
// SPDX-License-Identifier: MIT

#pragma once
#include "video_post_process.h"
#include <stdexcept>
#include <memory>

inline void ConvertRgbFrame(uint8_t* input, OutputSurfaceInfo* info, uint8_t* output,
                            OutputFormatEnum format, uint32_t pitch) {
    if (info->surface_format == rocDecVideoSurfaceFormat_NV12) {
        switch (format) {
        case bgr: Nv12ToColor24<BGR24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgr48: Nv12ToColor48<BGR48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb: Nv12ToColor24<RGB24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb48: Nv12ToColor48<RGB48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra: Nv12ToColor32<BGRA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra64: Nv12ToColor64<BGRA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba: Nv12ToColor32<RGBA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba64: Nv12ToColor64<RGBA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else if (info->surface_format == rocDecVideoSurfaceFormat_P016) {
        switch (format) {
        case bgr: P016ToColor24<BGR24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgr48: P016ToColor48<BGR48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb: P016ToColor24<RGB24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb48: P016ToColor48<RGB48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra: P016ToColor32<BGRA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra64: P016ToColor64<BGRA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba: P016ToColor32<RGBA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba64: P016ToColor64<RGBA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else if (info->surface_format == rocDecVideoSurfaceFormat_YUV444) {
        switch (format) {
        case bgr: YUV444ToColor24<BGR24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgr48: YUV444ToColor48<BGR48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb: YUV444ToColor24<RGB24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb48: YUV444ToColor48<RGB48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra: YUV444ToColor32<BGRA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra64: YUV444ToColor64<BGRA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba: YUV444ToColor32<RGBA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba64: YUV444ToColor64<RGBA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else if (info->surface_format == rocDecVideoSurfaceFormat_YUV444_16Bit) {
        switch (format) {
        case bgr: YUV444P16ToColor24<BGR24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgr48: YUV444P16ToColor48<BGR48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb: YUV444P16ToColor24<RGB24>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgb48: YUV444P16ToColor48<RGB48>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra: YUV444P16ToColor32<BGRA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case bgra64: YUV444P16ToColor64<BGRA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba: YUV444P16ToColor32<RGBA32>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        case rgba64: YUV444P16ToColor64<RGBA64>(input, info->output_pitch, output, pitch,
            info->output_width, info->output_height, info->output_vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else { throw std::invalid_argument("Unsupported YUV surface for RGB conversion"); }
}

#if ROCPYDECODE_ENABLE_HOST
// FFmpeg returns planar, low-bit-aligned samples. Expand chroma and align
// high-bit-depth samples before using rocDecode's existing RGB kernels.
template<class T>
__global__ static void ExpandPlanarYuv(const T* input, T* output, uint32_t width,
                                      uint32_t height, uint32_t stride,
                                      uint32_t vstride, int x_shift, int y_shift,
                                      int bit_shift) {
    const uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    const size_t plane = size_t(width) * height;
    const uint32_t chroma_stride = stride >> x_shift;
    const size_t chroma_plane = size_t(chroma_stride) * (vstride >> y_shift);
    const T* u = input + size_t(stride) * vstride;
    const T* v = u + chroma_plane;
    const size_t chroma_index = size_t(y >> y_shift) * chroma_stride + (x >> x_shift);
    const size_t index = size_t(y) * width + x;
    output[index] = input[size_t(y) * stride + x] << bit_shift;
    output[plane + index] = u[chroma_index] << bit_shift;
    output[2 * plane + index] = v[chroma_index] << bit_shift;
}

inline void ConvertCpuRgbFrame(uint8_t* input, OutputSurfaceInfo* info, uint8_t* output,
                               OutputFormatEnum format, uint32_t pitch) {
    auto allocate = [](size_t size) {
        void* ptr = nullptr;
        HIP_API_CALL(hipMalloc(&ptr, size));
        return std::shared_ptr<void>(ptr, [](void* p) { (void)hipFree(p); });
    };
    std::shared_ptr<void> staged;
    if (info->mem_type == OUT_SURFACE_MEM_HOST_COPIED) {
        staged = allocate(info->output_surface_size_in_bytes);
        HIP_API_CALL(hipMemcpy(staged.get(), input, info->output_surface_size_in_bytes,
                               hipMemcpyHostToDevice));
        input = static_cast<uint8_t*>(staged.get());
    }
    int x_shift = 0, y_shift = 0;
    switch (info->surface_format) {
    case rocDecVideoSurfaceFormat_YUV420:
    case rocDecVideoSurfaceFormat_YUV420_16Bit: x_shift = 1; y_shift = 1; break;
    case rocDecVideoSurfaceFormat_YUV422:
    case rocDecVideoSurfaceFormat_YUV422_16Bit: x_shift = 1; break;
    case rocDecVideoSurfaceFormat_YUV444:
    case rocDecVideoSurfaceFormat_YUV444_16Bit: break;
    default:
        ConvertRgbFrame(input, info, output, format, pitch);
        HIP_API_CALL(hipStreamSynchronize(0));
        return;
    }
    auto expanded = allocate(size_t(info->output_width) * info->output_height *
                             info->bytes_per_pixel * 3);
    const dim3 block(16, 16);
    const dim3 grid((info->output_width + 15) / 16, (info->output_height + 15) / 16);
    if (info->bytes_per_pixel == 1) {
        ExpandPlanarYuv<uint8_t><<<grid, block>>>(input, static_cast<uint8_t*>(expanded.get()),
            info->output_width, info->output_height, info->output_pitch,
            info->output_vstride, x_shift, y_shift, 0);
    } else {
        ExpandPlanarYuv<uint16_t><<<grid, block>>>(reinterpret_cast<uint16_t*>(input),
            static_cast<uint16_t*>(expanded.get()), info->output_width, info->output_height,
            info->output_pitch / 2, info->output_vstride, x_shift, y_shift, 16 - info->bit_depth);
    }
    HIP_API_CALL(hipGetLastError());
    OutputSurfaceInfo planar = *info;
    planar.output_pitch = info->output_width * info->bytes_per_pixel;
    planar.output_vstride = info->output_height;
    planar.surface_format = info->bytes_per_pixel == 1 ? rocDecVideoSurfaceFormat_YUV444 :
                                                       rocDecVideoSurfaceFormat_YUV444_16Bit;
    ConvertRgbFrame(static_cast<uint8_t*>(expanded.get()), &planar, output, format, pitch);
    HIP_API_CALL(hipStreamSynchronize(0));
}
#endif
