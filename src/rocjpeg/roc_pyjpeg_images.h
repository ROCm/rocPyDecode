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

#ifndef PY_ROC_JPEG_IMAGES_HEADER
#define PY_ROC_JPEG_IMAGES_HEADER
#pragma once

#include <cstring>
#include <iostream>
#include "rocjpeg/rocjpeg.h"
#include "common/roc_pybuffer.h"
#include "roc_pyjpeg_codestream.h"

// TODO: need to implement a case for YUV to add to the RGB and RGB-Planar

class PyJpegImages {

public:
    PyJpegImages() {
        // init GPU mem -python access
        ext_buf.push_back(std::make_shared<BufferInterface>());
        ext_buf.push_back(std::make_shared<BufferInterface>());
        ext_buf.push_back(std::make_shared<BufferInterface>());
        num_channels = 0;
        subsampling = ROCJPEG_CSS_UNKNOWN;
    }
    static void ExportToPython(py::module& m);
 
    // The image in the GPU MEM represented with dlpack via this ext_buf (for external buffer)
    std::vector<std::shared_ptr<BufferInterface>> ext_buf; // external buffer, a view on the GPU MEM of the decoded image

    // public to be accessed by python pybind
    int m_width = 0;
    int m_height = 0;
    py::array_t<uint8_t> to_numpy(int index = 0);
    RocJpegChromaSubsampling subsampling;

    // not exposed to outside
    uint32_t num_channels = 0;
    RocJpegImage output_image{};
    RocJpegDecodeParams decode_params{};
    bool ToDlpackTensor(RocJpegOutputFormat output_format, int device_id);

private:
    bool GetOutputDims(std::vector<uint32_t>& widths, std::vector<uint32_t>& heights, uint32_t img_width, uint32_t img_height, RocJpegOutputFormat output_format, RocJpegChromaSubsampling subsampling);
    [[maybe_unused]] uint32_t padding_ = 0;
};

#endif // PY_ROC_JPEG_IMAGES_HEADER
