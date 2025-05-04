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

#include <pybind11/pybind11.h>
 #include <pybind11/stl.h>
 #include "roc_pyselftest.h"
 #include "roc_pybuffer.h"
 #include "roc_pydlpack.h"

 namespace py = pybind11;
 
//  BufferInterface
void SelfTest_BufferInterface() {
    std::vector<uint8_t> host(4, 42);
    py::buffer_info bi(host.data(), sizeof(uint8_t),
                       py::format_descriptor<uint8_t>::format(),
                       2, {2, 2}, {2 * sizeof(uint8_t), sizeof(uint8_t)});
    DLDevice cpu{static_cast<DLDeviceType>(kDLCPU), 0};
    auto buf = std::make_shared<BufferInterface>(
        DLPackPyTensor(bi, cpu));

    (void)buf->shape();
    (void)buf->strides();
    (void)buf->dtype();
    (void)buf->data();
}

//  DLPackPyTensor 
void SelfTest_DLPackPyTensor() {
    std::vector<int32_t> host(8, 1);
    py::buffer_info bi(host.data(), sizeof(int32_t),
                       py::format_descriptor<int32_t>::format(),
                       1, {8}, {sizeof(int32_t)});
    DLDevice cpu{static_cast<DLDeviceType>(kDLCPU), 0};
    DLPackPyTensor tensor(bi, cpu);
    (void)(*tensor).device.device_type;
}

//  All tests
void RunAllSelfTests() {
    SelfTest_BufferInterface();
    SelfTest_DLPackPyTensor();
}