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

#include "roc_pycolorconversion.h"

using namespace std;

void PyColorConversionInitializer(py::module& m) {
    py::class_<PyRocColorConversion, std::shared_ptr<PyRocColorConversion>> (m, "PyResize")
    .def(py::init<>())
	.def("PyYUV444ToColor32",&PyRocColorConversion::PyYUV444ToColor32,"Convert YUV444 To Color32")
    .def("PyYUV444ToColor64",&PyRocColorConversion::PyYUV444ToColor64,"Convert YUV444 To Color64")
    .def("PyYUV444ToColor24",&PyRocColorConversion::PyYUV444ToColor24,"Convert YUV444 To Color24")
    .def("PyYUV444ToColor48",&PyRocColorConversion::PyYUV444ToColor48,"Convert YUV444 To Color48")
    .def("PyNv12ToColor24",&PyRocColorConversion::PyNv12ToColor24,"Convert Nv12 To Color24")
    .def("PyNv12ToColor32",&PyRocColorConversion::PyNv12ToColor32,"Convert Nv12 To Color32")
    .def("PyNv12ToColor48",&PyRocColorConversion::PyNv12ToColor48,"Convert Nv12 To Color48")
    .def("PyNv12ToColor64",&PyRocColorConversion::PyNv12ToColor64,"Convert Nv12 To Color64")
    .def("PyYUV444P16ToColor24",&PyRocColorConversion::PyYUV444P16ToColor24,"Convert YUV444P16 To Color24")
    .def("PyYUV444P16ToColor48",&PyRocColorConversion::PyYUV444P16ToColor48,"Convert YUV444P16 To Color48")
    .def("PyYUV444P16ToColor32",&PyRocColorConversion::PyYUV444P16ToColor32,"Convert YUV444P16 To Color32")
    .def("PyYUV444P16ToColor64",&PyRocColorConversion::PyYUV444P16ToColor64,"Convert YUV444P16 To Color64")
    .def("PyP016ToColor32",&PyRocColorConversion::PyP016ToColor32,"Convert P016 To Color32")
    .def("PyP016ToColor64",&PyRocColorConversion::PyP016ToColor64,"Convert P016 To Color64")
    .def("PyP016ToColor24",&PyRocColorConversion::PyP016ToColor24,"Convert P016 To Color24")
    .def("PyP016ToColor48",&PyRocColorConversion::PyP016ToColor48,"Convert P016 To Color48");
}

PyRocColorConversion::PyRocColorConversion() {

}

PyRocColorConversion::~PyRocColorConversion() {

}

void PyRocColorConversion::PyYUV444ToColor32(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444ToColor64(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444ToColor24(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444ToColor48(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyNv12ToColor24(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyNv12ToColor32(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyNv12ToColor48(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyNv12ToColor64(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444P16ToColor24(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444P16ToColor48(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444P16ToColor32(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyYUV444P16ToColor64(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyP016ToColor32(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyP016ToColor64(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyP016ToColor24(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

void PyRocColorConversion::PyP016ToColor48(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream) {

}

 