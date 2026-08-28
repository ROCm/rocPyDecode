/*
Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.

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

#ifndef ROC_PY_JPEG_UTILS
#define ROC_PY_JPEG_UTILS
#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <condition_variable>
#include <queue>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <chrono>
#include "rocjpeg/rocjpeg.h"

#define PY_CHECK_ROCJPEG(call)                                             \
    do {                                                                   \
        const RocJpegStatus rocjpeg_status = (call);                       \
        if (rocjpeg_status != ROCJPEG_STATUS_SUCCESS) {                    \
            std::cerr << #call << " returned " << rocJpegGetErrorName(rocjpeg_status) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (false)

#define PY_CHECK_HIP(call)                                                 \
    do {                                                                   \
        const hipError_t hip_status = (call);                              \
        if (hip_status != hipSuccess) {                                    \
            std::cout << "rocJPEG failure: '#" << hip_status << "' at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (false)

/**
 * @class PyRocJpegUtils
 * @brief Utility class for rocPyJpeg.
 *
 * This class provides utility functions such as getting file paths, initializing HIP device, 
 * getting chroma subsampling string, getting channel pitch and sizes, getting output file extension, and saving images.
 */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
class PyRocJpegUtils {
public:
    /**
     * @brief Initializes the HIP device.
     *
     * This function initializes the HIP device with the specified device ID.
     *
     * @param device_id The device ID.
     * @return True if successful, false otherwise.
     */
    std::tuple<int, bool> InitHipDevice(int device_id, bool display_prop = true) {
        int num_devices;
        hipDeviceProp_t hip_dev_prop;
        PY_CHECK_HIP(hipGetDeviceCount(&num_devices));
        if (num_devices < 1) {
            std::cerr << "ERROR: didn't find any GPU!" << std::endl;
            return std::make_tuple(num_devices, false);
        }
        // if '-' then caller needs only count of devices
        if(device_id<0)
            return std::make_tuple(num_devices, true);
        if (device_id >= num_devices) {
            std::cerr << "ERROR: the requested device_id is not found!" << std::endl;
            return std::make_tuple(num_devices, false);
        }
        PY_CHECK_HIP(hipSetDevice(device_id));

        if(display_prop) {
            PY_CHECK_HIP(hipGetDeviceProperties(&hip_dev_prop, device_id));
            std::cout << "Using GPU device " << device_id << ": " << hip_dev_prop.name << "[" << hip_dev_prop.gcnArchName << "] on PCI bus " <<
            std::setfill('0') << std::setw(2) << std::right << std::hex << hip_dev_prop.pciBusID << ":" << std::setfill('0') << std::setw(2) <<
            std::right << std::hex << hip_dev_prop.pciDomainID << "." << hip_dev_prop.pciDeviceID << std::dec << std::endl;
        }
        return std::make_tuple(num_devices, true);
    }
    /**
     * @brief Gets the chroma subsampling string.
     *
     * This function gets the chroma subsampling string based on the specified subsampling value.
     *
     * @param subsampling The chroma subsampling value.
     * @param chroma_sub_sampling The string to store the chroma subsampling.
     */
    void GetChromaSubsamplingStr(RocJpegChromaSubsampling subsampling, std::string &chroma_sub_sampling) {
        switch (subsampling) {
            case ROCJPEG_CSS_444:
                chroma_sub_sampling = "YUV 4:4:4";
                break;
            case ROCJPEG_CSS_440:
                chroma_sub_sampling = "YUV 4:4:0";
                break;
            case ROCJPEG_CSS_422:
                chroma_sub_sampling = "YUV 4:2:2";
                break;
            case ROCJPEG_CSS_420:
                chroma_sub_sampling = "YUV 4:2:0";
                break;
            case ROCJPEG_CSS_411:
                chroma_sub_sampling = "YUV 4:1:1";
                break;
            case ROCJPEG_CSS_400:
                chroma_sub_sampling = "YUV 4:0:0";
                break;
            case ROCJPEG_CSS_UNKNOWN:
                chroma_sub_sampling = "UNKNOWN";
                break;
            default:
                chroma_sub_sampling = "UNKNOWN";
                break;
        }
    }

    /**
     * @brief Gets the channel pitch and sizes.
     *
     * This function gets the channel pitch and sizes based on the specified output format, chroma subsampling,
     * output image, and channel sizes.
     *
     * @param decode_params The decode parameters that specify the output format and crop rectangle.
     * @param subsampling The chroma subsampling.
     * @param widths The array to store the channel widths.
     * @param heights The array to store the channel heights.
     * @param num_channels The number of channels.
     * @param output_image The output image.
     * @param channel_sizes The array to store the channel sizes.
     * @return The channel pitch.
     */
    int GetChannelPitchAndSizes(RocJpegDecodeParams decode_params, RocJpegChromaSubsampling subsampling,
                                const std::vector<uint32_t> &widths, const std::vector<uint32_t> &heights,
                                uint32_t &num_channels, RocJpegImage &output_image,
                                std::vector<uint32_t> &channel_sizes) {
        const int roi_width_raw = decode_params.crop_rectangle.right - decode_params.crop_rectangle.left;
        const int roi_height_raw = decode_params.crop_rectangle.bottom - decode_params.crop_rectangle.top;
        const uint32_t roi_width = static_cast<uint32_t>(roi_width_raw);
        const uint32_t roi_height = static_cast<uint32_t>(roi_height_raw);
        const bool is_roi_valid = roi_width_raw > 0 && roi_height_raw > 0 && roi_width <= widths[0] && roi_height <= heights[0];
        const uint32_t full_width = is_roi_valid ? roi_width : widths[0];
        const uint32_t full_height = is_roi_valid ? roi_height : heights[0];
        std::vector<uint32_t> pitches(ROCJPEG_MAX_COMPONENT, 0U);
        std::fill(channel_sizes.begin(), channel_sizes.end(), 0U);

        const auto set_channel = [&](std::size_t index, uint32_t pitch, uint32_t height) {
            pitches[index] = pitch;
            channel_sizes[index] = AlignSize(pitch, height, mem_alignment);
        };

        switch (decode_params.output_format) {
            case ROCJPEG_OUTPUT_NATIVE:
                switch (subsampling) {
                    case ROCJPEG_CSS_444:
                        num_channels = 3U;
                        set_channel(0U, full_width, full_height);
                        set_channel(1U, full_width, full_height);
                        set_channel(2U, full_width, full_height);
                        break;
                    case ROCJPEG_CSS_440:
                        num_channels = 3U;
                        set_channel(0U, full_width, full_height);
                        set_channel(1U, full_width, full_height >> 1U);
                        set_channel(2U, full_width, full_height >> 1U);
                        break;
                    case ROCJPEG_CSS_422:
                        num_channels = 1U;
                        set_channel(0U, full_width * 2U, full_height);
                        break;
                    case ROCJPEG_CSS_420:
                        num_channels = 2U;
                        set_channel(0U, full_width, full_height);
                        set_channel(1U, full_width, full_height >> 1U);
                        break;
                    case ROCJPEG_CSS_400:
                        num_channels = 1U;
                        set_channel(0U, full_width, full_height);
                        break;
                    case ROCJPEG_CSS_411:
                    case ROCJPEG_CSS_UNKNOWN:
                        std::cout << "Unknown chroma subsampling!" << std::endl;
                        return EXIT_FAILURE;
                    default:
                        std::cout << "Unknown chroma subsampling!" << std::endl;
                        return EXIT_FAILURE;
                }
                break;
            case ROCJPEG_OUTPUT_YUV_PLANAR:
                if (subsampling == ROCJPEG_CSS_400) {
                    num_channels = 1U;
                    set_channel(0U, full_width, full_height);
                } else {
                    switch (subsampling) {
                        case ROCJPEG_CSS_444:
                            num_channels = 3U;
                            set_channel(0U, full_width, full_height);
                            set_channel(1U, full_width, full_height);
                            set_channel(2U, full_width, full_height);
                            break;
                        case ROCJPEG_CSS_440: {
                            const uint32_t chroma_height = is_roi_valid ? (full_height >> 1U) : heights[1];
                            num_channels = 3U;
                            set_channel(0U, full_width, full_height);
                            set_channel(1U, full_width, chroma_height);
                            set_channel(2U, full_width, chroma_height);
                            break;
                        }
                        case ROCJPEG_CSS_422: {
                            const uint32_t chroma_width = is_roi_valid ? (full_width >> 1U) : widths[1];
                            num_channels = 3U;
                            set_channel(0U, full_width, full_height);
                            set_channel(1U, chroma_width, full_height);
                            set_channel(2U, chroma_width, full_height);
                            break;
                        }
                        case ROCJPEG_CSS_420: {
                            const uint32_t chroma_width = is_roi_valid ? (full_width >> 1U) : widths[1];
                            const uint32_t chroma_height = is_roi_valid ? (full_height >> 1U) : heights[1];
                            num_channels = 3U;
                            set_channel(0U, full_width, full_height);
                            set_channel(1U, chroma_width, chroma_height);
                            set_channel(2U, chroma_width, chroma_height);
                            break;
                        }
                        case ROCJPEG_CSS_400:
                            break;
                        case ROCJPEG_CSS_411:
                        case ROCJPEG_CSS_UNKNOWN:
                            std::cout << "Unknown chroma subsampling!" << std::endl;
                            return EXIT_FAILURE;
                        default:
                            std::cout << "Unknown chroma subsampling!" << std::endl;
                            return EXIT_FAILURE;
                    }
                }
                break;
            case ROCJPEG_OUTPUT_Y:
                num_channels = 1U;
                set_channel(0U, full_width, full_height);
                break;
            case ROCJPEG_OUTPUT_RGB:
                num_channels = 1U;
                set_channel(0U, full_width * 3U, full_height);
                break;
            case ROCJPEG_OUTPUT_RGB_PLANAR:
                num_channels = 3U;
                set_channel(0U, full_width, full_height);
                set_channel(1U, full_width, full_height);
                set_channel(2U, full_width, full_height);
                break;
            case ROCJPEG_OUTPUT_FORMAT_MAX:
                std::cout << "Unknown output format!" << std::endl;
                return EXIT_FAILURE;
            default:
                std::cout << "Unknown output format!" << std::endl;
                return EXIT_FAILURE;
        }
        std::copy(pitches.begin(), pitches.end(), std::begin(output_image.pitch));
        return EXIT_SUCCESS;
    }

private:
    static constexpr uint32_t mem_alignment = 4U * 1024U * 1024U;
    /**
     * @brief Aligns a value to a specified alignment.
     *
     * This function takes a value and aligns it to the specified alignment. It returns the aligned value.
     *
     * @param pitch The pitch of the channel in bytes.
     * @param height The channel height in rows.
     * @param alignment The alignment value.
     * @return The aligned value.
     */
    static inline uint32_t AlignSize(uint32_t pitch, uint32_t height, uint32_t alignment) {
        const auto size = static_cast<uint64_t>(pitch) * static_cast<uint64_t>(height);
        const auto aligned = (size + alignment - 1U) & ~(static_cast<uint64_t>(alignment) - 1U);
        return static_cast<uint32_t>(aligned);
    }
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif //ROC_PY_JPEG_UTILS
