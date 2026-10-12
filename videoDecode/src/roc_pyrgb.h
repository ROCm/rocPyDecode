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
