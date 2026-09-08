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

#include <iostream>
#include "rocjpeg/rocjpeg.h"
#include "roc_pybuffer.h"
#include "roc_pyjpeg_images.h"
#include "roc_pyjpeg.h"

namespace py = pybind11;
using namespace py::literals;

using namespace std;

#include <pybind11/numpy.h>

void PyJpegImages::ExportToPython(py::module& m) {
    // PyJpegImages
    py::class_<PyJpegImages, shared_ptr<PyJpegImages>>(m, "PyJpegImages", py::module_local())
        .def(py::init<>())
        .def_readwrite("ext_buf", &PyJpegImages::ext_buf)
        .def("to_numpy", &PyJpegImages::to_numpy, py::arg("index") = 0, "Export a given plane as a 'numpy' uint 8 bits array")
        // DL Pack Tensor
        .def_property_readonly("shapeY", [](std::shared_ptr<PyJpegImages>& self) {
            return self->ext_buf[0]->shape();
            }, "Get the shape of the Y plane buffer as an array")
        .def_property_readonly("shapeUV", [](std::shared_ptr<PyJpegImages>& self) {
            return self->ext_buf[1]->shape();
            }, "Get the shape of the U plane buffer as an array")
        .def_property_readonly("shapeU", [](std::shared_ptr<PyJpegImages>& self) {
            return self->ext_buf[1]->shape();
            }, "Get the shape of the U plane buffer as an array")
        .def_property_readonly("shapeV", [](std::shared_ptr<PyJpegImages>& self) {
            return self->ext_buf[2]->shape();
            }, "Get the shape of the V plane buffer as an array")
        .def_property_readonly("shape", [](std::shared_ptr<PyJpegImages>& self) {
            return self->ext_buf[0]->shape();
            }, "Get the shape of the buffer as an array")
        .def_property_readonly("strides", [](std::shared_ptr<PyJpegImages>& self) {
                return self->ext_buf[0]->strides();
            }, "Get the strides of the buffer")
        .def_property_readonly("dtype", [](std::shared_ptr<PyJpegImages>& self) {
                return self->ext_buf[0]->dtype();
            }, "Get the data type of the buffer")
        .def("__dlpack__", [](std::shared_ptr<PyJpegImages>& self, py::object stream) {
            return self->ext_buf[0]->dlpack(stream);
            }, py::arg("stream") = NULL, "Export the buffer as a DLPack tensor")
        .def("__dlpack_device__", [](std::shared_ptr<PyJpegImages>& self) {
                return py::make_tuple(py::int_(static_cast<int>(DLDeviceType::kDLROCM)), py::int_(static_cast<int>(0)));
            }, "Get the device associated with the buffer")
        .def_readwrite("height", &PyJpegImages::m_height, 
            R"pbdoc(
            The vertical dimension of the entire image in pixels.
            )pbdoc")
        .def_readwrite("width", &PyJpegImages::m_width, 
            R"pbdoc(
            The horizontal dimension of the entire image in pixels.
            )pbdoc");
}

py::array_t<uint8_t> PyJpegImages::to_numpy(int index) {
    py::array_t<uint8_t> ret;
    if (index < 0 || index >= static_cast<int>(ext_buf.size()))
        throw std::out_of_range("Invalid channel index");
    auto& buf = ext_buf[index];
    uint8_t* data_ptr = static_cast<uint8_t*>(buf->data());
    py::tuple py_shape = buf->shape();
    if (py_shape.size() == 3) {
        const ssize_t height   = py_shape[0].cast<ssize_t>();
        const ssize_t width    = py_shape[1].cast<ssize_t>();
        const ssize_t channels = py_shape[2].cast<ssize_t>();
        std::vector<ssize_t> shape   = { height, width, channels };
        std::vector<ssize_t> strides = { width * channels, channels, 1 };
        ret = py::array_t<uint8_t>(shape, strides, data_ptr, py::cast(buf));
    } else if (py_shape.size() == 2) {
        const ssize_t height = py_shape[0].cast<ssize_t>();
        const ssize_t width  = py_shape[1].cast<ssize_t>();
        const ssize_t row_stride = buf->strides()[0].cast<ssize_t>();
        std::vector<ssize_t> shape   = { height, width };
        std::vector<ssize_t> strides = { row_stride, 1 };
        ret = py::array_t<uint8_t>(shape, strides, data_ptr, py::cast(buf));
    } else {
        throw std::runtime_error("Unsupported shape: only 2D or 3D supported");
    }
    return ret;
}

bool PyJpegImages::GetOutputDims(std::vector<uint32_t>& widths, std::vector<uint32_t>& heights, 
                                uint32_t img_width, uint32_t img_height, RocJpegOutputFormat output_format, 
                                RocJpegChromaSubsampling subsampling) {
    switch (output_format) {
        case ROCJPEG_OUTPUT_NATIVE:
            switch (subsampling) {
                case ROCJPEG_CSS_444:
                    widths[2] = widths[1] = widths[0] = img_width;
                    heights[2] = heights[1] = heights[0] = img_height;
                    break;
                case ROCJPEG_CSS_440:
                    widths[2] = widths[1] = widths[0] = img_width;
                    heights[0] = img_height;
                    heights[2] = heights[1] = img_height >> 1;
                    break;
                case ROCJPEG_CSS_422:
                    widths[0] = img_width * 2;
                    heights[0] = img_height;
                    break;
                case ROCJPEG_CSS_420:
                    widths[1] = widths[0] = img_width;
                    heights[0] = img_height;
                    heights[1] = img_height >> 1;
                    break;
                case ROCJPEG_CSS_400:
                    widths[0] = img_width;
                    heights[0] = img_height;
                    break;
                default:
                    std::cout << "Unknown chroma subsampling!" << std::endl;
                    return false;
            }
            break;
        case ROCJPEG_OUTPUT_YUV_PLANAR:
            switch (subsampling) {
                case ROCJPEG_CSS_444:
                    widths[2] = widths[1] = widths[0] = img_width;
                    heights[2] = heights[1] = heights[0] = img_height;
                    break;
                case ROCJPEG_CSS_440:
                    widths[2] = widths[1] = widths[0] = img_width;
                    heights[0] = img_height;
                    heights[2] = heights[1] = img_height >> 1;
                    break;
                case ROCJPEG_CSS_422:
                    widths[0] = img_width;
                    widths[2] = widths[1] = widths[0] >> 1;
                    heights[2] = heights[1] = heights[0] = img_height;
                    break;
                case ROCJPEG_CSS_420:
                    widths[0] = img_width;
                    widths[2] = widths[1] = widths[0] >> 1;
                    heights[0] = img_height;
                    heights[2] = heights[1] = img_height >> 1;
                    break;
                case ROCJPEG_CSS_400:
                    widths[0] = img_width;
                    heights[0] = img_height;
                    break;
                default:
                    std::cout << "Unknown chroma subsampling!" << std::endl;
                    return false;
            }
            break;
        case ROCJPEG_OUTPUT_Y:
            widths[0] = img_width;
            heights[0] = img_height;
            break;
        case ROCJPEG_OUTPUT_RGB:
            widths[0] = img_width * 3;
            heights[0] = img_height;
            break;
        case ROCJPEG_OUTPUT_RGB_PLANAR:
            widths[2] = widths[1] = widths[0] = img_width;
            heights[2] = heights[1] = heights[0] = img_height;
            break;
        default:
            std::cout << "Unknown output format!" << std::endl;
            return false;
    }
    return true;
}

bool PyJpegImages::ToDlpackTensor(RocJpegOutputFormat output_format, int device_id) {
    uint32_t img_width = m_width;
    uint32_t img_height = m_height;    
    std::vector<uint32_t> widths;
    std::vector<uint32_t> heights;
    widths.resize(ROCJPEG_MAX_COMPONENT);
    heights.resize(ROCJPEG_MAX_COMPONENT);
     if(GetOutputDims(widths, heights, img_width, img_height, output_format, subsampling) == false)
        return false;
    uint32_t bit_depth = 8;
    std::string type_str(static_cast<const char*>("|u1"));
    switch(output_format) {
        case ROCJPEG_OUTPUT_RGB_PLANAR: { // each color plane in a channel separately R[0], G[1], and B[2]
            uint32_t surf_stride[3] = {widths[0], widths[1], widths[2]}; // ROCJPEG_OUTPUT_RGB_PLANAR all same width = img_width
            for(int i = 0; i < 3; i++) {
                std::vector<size_t> shape{ static_cast<size_t>(heights[i]), static_cast<size_t>(widths[i])}; // depend on get_output_dims()
                std::vector<size_t> stride{ static_cast<size_t>(surf_stride[i]), 1, 0};
                // RGB PLANAR using VCN JPEG decoder @ first, second, and third channel of RocJpegImage
                ext_buf[i]->LoadDLPack(shape, stride, bit_depth, type_str, (void *)output_image.channel[i], device_id); // device_id was set/saved at the constructor
            }
        }
        break;
        default:
        case ROCJPEG_OUTPUT_RGB: { // all the RGB interleaved in one channel [0]
            uint32_t surf_stride = widths[0]; // ROCJPEG_OUTPUT_RGB width is * 3 for RGB interleaved
            std::vector<size_t> shape{ static_cast<size_t>(heights[0]), static_cast<size_t>(widths[0]/3), 3}; // widths[0]/3 for ROCJPEG_OUTPUT_RGB
            std::vector<size_t> stride{ static_cast<size_t>(surf_stride), 1, 0}; // python assumes same dim for both shape & strides
            // interleaved RGB using VCN JPEG decoder written to first channel of RocJpegImage
            ext_buf[0]->LoadDLPack(shape, stride, bit_depth, type_str, (void *)output_image.channel[0], device_id); // device_id was set/saved at the constructor
        }
        break;
    }
    return true;
}
