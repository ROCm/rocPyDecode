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

#pragma once

#include "roc_video_dec.h"
#include "roc_pydecode.h"
#include "resize_kernels.h"

//
// AMD HIP Color Conversion Python Interface class
//
class PyResizeFrame {

    public:

        PyResizeFrame();
        ~PyResizeFrame();

        // (pass void * as hipStream_t, then cast void * to hipStream_t)
        void PyResizeNv12(uint8_t *p_dst_nv12, int dst_pitch, int dst_width, int dst_height, uint8_t *p_src_nv12, 
                        int src_pitch, int src_width, int src_height, unsigned char* p_src_nv12_uv, unsigned char* p_dst_nv12_uv, void * hip_stream);
        
        void PyResizeP016(uint8_t *p_dst_p016, int dst_pitch, int dst_width, int dst_height, uint8_t *p_src_p016, int src_pitch, 
                    int src_width, int src_height, unsigned char* p_src_p016_uv, unsigned char* p_dst_p016_uv, void * hip_stream);
        
        void PyResizeYUV420(uint8_t *p_dst_y, uint8_t* p_dst_u, uint8_t* p_dst_v, int dst_pitch_y, int dst_pitch_uv, 
                        int dst_width, int dst_height, uint8_t *p_src_y, uint8_t* p_src_u, uint8_t* p_src_v,
                        int src_pitch_y, int src_pitch_uv, int src_width, int src_height, bool b_nv12 = false, void * hip_stream = nullptr);
        
        void PyResizeYUVHipLaunchKernel(uint8_t *dp_dst, int dst_pitch, int dst_width, int dst_height, uint8_t *dp_src, int src_pitch, 
                                            int src_width, int src_height, bool b_resize_uv = false, void * hip_stream = nullptr);

};