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
#include "colorspace_kernels.h"
#include "resize_kernels.h"
#include "roc_pydecode.h"

//
// AMD HIP Color Conversion Python Interface class
//
class PyRocColorConversion {

    public:

        PyRocColorConversion();
        ~PyRocColorConversion();

        // color-convert hip kernel function definitions (pass void * as hipStream_t, then cast void * to hipStream_t)
        void PyYUV444ToColor32(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444ToColor64(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444ToColor24(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444ToColor48(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyNv12ToColor24(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyNv12ToColor32(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyNv12ToColor48(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyNv12ToColor64(uint8_t *dp_nv12, int nv12_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444P16ToColor24(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444P16ToColor48(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444P16ToColor32(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyYUV444P16ToColor64(uint8_t *dp_yuv_444, int pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyP016ToColor32(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyP016ToColor64(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgra, int bgra_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyP016ToColor24(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
        void PyP016ToColor48(uint8_t *dp_p016, int p016_pitch, uint8_t *dp_bgr, int bgr_pitch, int width, int height, int v_pitch, int col_standard, void * hip_stream);
};