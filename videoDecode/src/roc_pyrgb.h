// Copyright © Advanced Micro Devices, Inc., or its affiliates.
// SPDX-License-Identifier: MIT

#pragma once
#include "video_post_process.h"
#include <stdexcept>
#include <memory>
#include <limits>

inline uint32_t CalculateRgbPitch(uint32_t width, OutputFormatEnum output_format) {
    const int format = static_cast<int>(output_format);
    if (format < 1 || format > 8)
        throw std::invalid_argument("RGB format must be in the range 1 through 8");
    const uint32_t channels = format >= 5 ? 4u : 3u;
    const uint32_t item_size = format % 2 == 0 ? 2u : 1u;
    const uint64_t pitch = ((uint64_t{width} + 1) & ~uint64_t{1}) * channels * item_size;
    if (pitch > static_cast<uint64_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument("RGB pitch exceeds the SDK integer range");
    return static_cast<uint32_t>(pitch);
}

inline void ConvertDeviceRgbFrame(uint8_t* input, OutputSurfaceInfo* info, uint8_t* output,
                            OutputFormatEnum format, uint32_t pitch) {
    if (info->output_width > std::numeric_limits<int>::max() ||
        info->output_height > std::numeric_limits<int>::max() ||
        info->output_vstride > std::numeric_limits<int>::max() ||
        info->output_pitch > std::numeric_limits<int>::max() ||
        pitch > std::numeric_limits<int>::max())
        throw std::invalid_argument("RGB dimensions and pitches exceed the SDK integer range");
    const int width = static_cast<int>(info->output_width);
    const int height = static_cast<int>(info->output_height);
    const int vstride = static_cast<int>(info->output_vstride);
    const int input_pitch = static_cast<int>(info->output_pitch);
    const int output_pitch = static_cast<int>(pitch);
    if (info->surface_format == rocDecVideoSurfaceFormat_NV12) {
        switch (format) {
        case bgr: Nv12ToColor24<BGR24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgr48: Nv12ToColor48<BGR48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb: Nv12ToColor24<RGB24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb48: Nv12ToColor48<RGB48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra: Nv12ToColor32<BGRA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra64: Nv12ToColor64<BGRA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba: Nv12ToColor32<RGBA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba64: Nv12ToColor64<RGBA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else if (info->surface_format == rocDecVideoSurfaceFormat_P016) {
        switch (format) {
        case bgr: P016ToColor24<BGR24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgr48: P016ToColor48<BGR48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb: P016ToColor24<RGB24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb48: P016ToColor48<RGB48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra: P016ToColor32<BGRA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra64: P016ToColor64<BGRA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba: P016ToColor32<RGBA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba64: P016ToColor64<RGBA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else if (info->surface_format == rocDecVideoSurfaceFormat_YUV444) {
        switch (format) {
        case bgr: YUV444ToColor24<BGR24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgr48: YUV444ToColor48<BGR48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb: YUV444ToColor24<RGB24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb48: YUV444ToColor48<RGB48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra: YUV444ToColor32<BGRA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra64: YUV444ToColor64<BGRA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba: YUV444ToColor32<RGBA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba64: YUV444ToColor64<RGBA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else if (info->surface_format == rocDecVideoSurfaceFormat_YUV444_16Bit) {
        switch (format) {
        case bgr: YUV444P16ToColor24<BGR24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgr48: YUV444P16ToColor48<BGR48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb: YUV444P16ToColor24<RGB24>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgb48: YUV444P16ToColor48<RGB48>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra: YUV444P16ToColor32<BGRA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case bgra64: YUV444P16ToColor64<BGRA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba: YUV444P16ToColor32<RGBA32>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        case rgba64: YUV444P16ToColor64<RGBA64>(input, input_pitch, output, output_pitch,
            width, height, vstride, 0, 0); break;
        default: throw std::invalid_argument("Unsupported RGB format");
        }
    }
    else { throw std::invalid_argument("Unsupported YUV surface for RGB conversion"); }
}

inline void ConvertRgbFrame(uint8_t* input, OutputSurfaceInfo* info, uint8_t* output,
                            OutputFormatEnum format, uint32_t pitch) {
    std::shared_ptr<void> staged;
    auto device_info = *info;
    if (info->mem_type == OUT_SURFACE_MEM_HOST_COPIED) {
        void* ptr = nullptr;
        HIP_API_CALL(hipMalloc(&ptr, info->output_surface_size_in_bytes));
        staged = std::shared_ptr<void>(ptr, [](void* p) { (void)hipFree(p); });
        HIP_API_CALL(hipMemcpy(ptr, input, info->output_surface_size_in_bytes, hipMemcpyHostToDevice));
        input = static_cast<uint8_t*>(ptr);
        device_info.mem_type = OUT_SURFACE_MEM_DEV_COPIED;
    }
    ConvertDeviceRgbFrame(input, &device_info, output, format, pitch);
    HIP_API_CALL(hipStreamSynchronize(0));
}

// CPU decoders return planar, low-bit-aligned samples. Expand chroma and align
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
    output[index] = static_cast<T>(input[size_t(y) * stride + x] << bit_shift);
    output[plane + index] = static_cast<T>(u[chroma_index] << bit_shift);
    output[2 * plane + index] = static_cast<T>(v[chroma_index] << bit_shift);
}

// Full-range planar samples have no video-range luma offset. Pass coefficients
// as kernel arguments so concurrent CPU decoders do not share a color matrix.
template<class T, class Out>
__global__ static void FullRangePlanarToRgb(const T* input, Out* output,
        uint32_t width, uint32_t height, uint32_t stride, uint32_t vstride,
        uint32_t pitch, int x_shift, int y_shift, int depth, int channels,
        bool bgr_order, float wr, float wb) {
    const uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    const uint32_t chroma_stride = stride >> x_shift;
    const T* u = input + size_t(stride) * vstride;
    const T* v = u + size_t(chroma_stride) * (vstride >> y_shift);
    const size_t index = size_t(y >> y_shift) * chroma_stride + (x >> x_shift);
    const float fy = input[size_t(y) * stride + x];
    const float fu = float(u[index]) - (1 << (depth - 1));
    const float fv = float(v[index]) - (1 << (depth - 1));
    const float maximum = float((1u << depth) - 1);
    const float values[3] = {
        fy + 2 * (1 - wr) * fv,
        fy - 2 * wb * (1 - wb) / (1 - wr - wb) * fu
           - 2 * wr * (1 - wr) / (1 - wr - wb) * fv,
        fy + 2 * (1 - wb) * fu};
    Out* pixel = output + size_t(y) * pitch + size_t(x) * channels;
    for (int c = 0; c < 3; ++c) {
        // Match the existing RGB API's high-bit alignment for 16-bit output.
        uint32_t value = static_cast<uint32_t>(fminf(maximum, fmaxf(0.0f, values[c])));
        if (depth > int(sizeof(Out) * 8)) value >>= depth - sizeof(Out) * 8;
        else value <<= sizeof(Out) * 8 - depth;
        pixel[bgr_order ? 2 - c : c] = static_cast<Out>(value);
    }
    if (channels == 4) pixel[3] = 0;  // Same unused alpha channel as SDK conversions.
}

inline void ConvertCpuRgbFrame(uint8_t* input, OutputSurfaceInfo* info, uint8_t* output,
                               OutputFormatEnum format, uint32_t pitch,
                               bool full_range, int color_space) {
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
    auto device_info = *info;
    device_info.mem_type = OUT_SURFACE_MEM_DEV_COPIED;
    info = &device_info;
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
    if (full_range) {
        // Unspecified JPEG color space uses BT.601; honor explicitly tagged matrices.
        float wr = 0.299f, wb = 0.114f;
        switch (color_space) {
        case 1: wr = 0.2126f; wb = 0.0722f; break;  // BT.709
        case 4: wr = 0.30f; wb = 0.11f; break;      // FCC
        case 7: wr = 0.212f; wb = 0.087f; break;    // SMPTE 240M
        case 9: wr = 0.2627f; wb = 0.0593f; break;  // BT.2020 non-constant luminance
        case 0: case 2: case 5: case 6: break;
        default: throw std::invalid_argument("Unsupported full-range CPU color matrix");
        }
        const int channels = format >= 5 ? 4 : 3;
        const bool bgr_order = format == bgr || format == bgr48 || format == bgra || format == bgra64;
        const dim3 block(16, 16);
        const dim3 grid((info->output_width + 15) / 16, (info->output_height + 15) / 16);
        auto convert = [&](auto sample, auto pixel) {
            using T = decltype(sample);
            using Out = decltype(pixel);
            FullRangePlanarToRgb<T, Out><<<grid, block>>>(reinterpret_cast<T*>(input),
                reinterpret_cast<Out*>(output), info->output_width, info->output_height,
                info->output_pitch / sizeof(T), info->output_vstride, pitch / sizeof(Out),
                x_shift, y_shift, static_cast<int>(info->bit_depth), channels, bgr_order, wr, wb);
        };
        if (info->bytes_per_pixel == 1) {
            if (format % 2) convert(uint8_t{}, uint8_t{});
            else convert(uint8_t{}, uint16_t{});
        } else {
            if (format % 2) convert(uint16_t{}, uint8_t{});
            else convert(uint16_t{}, uint16_t{});
        }
        HIP_API_CALL(hipGetLastError());
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
            info->output_pitch / 2, info->output_vstride, x_shift, y_shift, 16 - static_cast<int>(info->bit_depth));
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
