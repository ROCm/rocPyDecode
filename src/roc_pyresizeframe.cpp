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

#include "roc_pyresizeframe.h"

using namespace std;

void PyResizeInitializer(py::module& m) {
    py::class_<PyResizeFrame, std::shared_ptr<PyResizeFrame>> (m, "PyResizeFrame")
    .def(py::init<>())
    .def("PyResizeNv12",&PyResizeFrame::PyResizeNv12,"Resize Nv12 Frame")
    .def("PyResizeP016",&PyResizeFrame::PyResizeP016,"Resize P016 Frame")
    .def("PyResizeYUV420",&PyResizeFrame::PyResizeYUV420,"Resize YUV420 Frame")
    .def("PyResizeYUVHipLaunchKernel",&PyResizeFrame::PyResizeYUVHipLaunchKernel,"Resize YUV Hip Launch Kernel");
        
}

PyResizeFrame::PyResizeFrame() {

}

PyResizeFrame::~PyResizeFrame() {

}

void PyResizeFrame::PyResizeNv12(uint8_t *p_dst_nv12, int dst_pitch, int dst_width, int dst_height, uint8_t *p_src_nv12, 
                int src_pitch, int src_width, int src_height, unsigned char* p_src_nv12_uv, unsigned char* p_dst_nv12_uv, void * hip_stream) {    
    ResizeNv12(p_dst_nv12, dst_pitch, dst_width, dst_height, p_src_nv12, 
                 src_pitch, src_width, src_height, p_src_nv12_uv, p_dst_nv12_uv, (hipStream_t) hip_stream);
}

void PyResizeFrame::PyResizeP016(uint8_t *p_dst_p016, int dst_pitch, int dst_width, int dst_height, uint8_t *p_src_p016, int src_pitch, 
            int src_width, int src_height, unsigned char* p_src_p016_uv, unsigned char* p_dst_p016_uv, void * hip_stream) {
    ResizeP016(p_dst_p016, dst_pitch, dst_width, dst_height, p_src_p016, src_pitch, 
                src_width, src_height, p_src_p016_uv, p_dst_p016_uv, (hipStream_t) hip_stream);
}

void PyResizeFrame::PyResizeYUV420(uint8_t *p_dst_y, uint8_t* p_dst_u, uint8_t* p_dst_v, int dst_pitch_y, int dst_pitch_uv, 
                int dst_width, int dst_height, uint8_t *p_src_y, uint8_t* p_src_u, uint8_t* p_src_v,
                int src_pitch_y, int src_pitch_uv, int src_width, int src_height, bool b_nv12, void * hip_stream) {
    ResizeYUV420(p_dst_y, p_dst_u, p_dst_v, dst_pitch_y, dst_pitch_uv, 
                dst_width, dst_height, p_src_y, p_src_u, p_src_v,
                src_pitch_y, src_pitch_uv, src_width, src_height, b_nv12, (hipStream_t) hip_stream);
}

void PyResizeFrame::PyResizeYUVHipLaunchKernel(uint8_t *dp_dst, int dst_pitch, int dst_width, int dst_height, uint8_t *dp_src, int src_pitch, 
                                    int src_width, int src_height, bool b_resize_uv, void * hip_stream) {
    ResizeYUVHipLaunchKernel(dp_dst, dst_pitch, dst_width, dst_height, dp_src, src_pitch, 
                src_width, src_height, b_resize_uv, (hipStream_t) hip_stream);
}
