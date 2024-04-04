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

#include "roc_pygpualloc.h"

using namespace std;

namespace py = pybind11;

static void* do_allocate(void* ctx, size_t bytes, size_t alignment)
{
    std::cout << "external malloc called" << std::endl;
    void* pFrame = NULL;
    HIP_ERROR_CHECK_STATUS(hipMalloc(&pFrame, bytes));
    return pFrame;
}
static int do_deallocate(void* ctx, void* pFrame, size_t bytes, size_t alignment)
{
    HIP_ERROR_CHECK_STATUS(hipFree(pFrame));
    return 0;
}

typedef void* (*do_allocate_pfn)(void* ctx, size_t bytes, size_t alignment);
typedef int (*do_deallocate_pfn)(void* ctx, void* pFrame, size_t bytes, size_t alignment);

PyGpuAlloc::PyGpuAlloc()
{
}

PYBIND11_MODULE(_PyGpuAlloc, m)
{
    do_allocate_pfn do_allocate_v1 = (do_allocate);
    do_deallocate_pfn do_deallocate_v1 = (do_deallocate);
    using do_allocate_v2 = std::function<void* (void*, size_t, size_t)>;
    using do_deallocate_v2 = std::function<int(void*, void*,size_t, size_t)>;

    m.def("do_allocate", [=]() {
        std::cout << "setting up internal allocator" << std::endl;
        return do_allocate_v2(do_allocate_v1);
        });
    m.def("do_deallocate", [=]() {
        std::cout << "setting up internal deallocator" << std::endl;
        return do_deallocate_v2(do_deallocate_v1);
    });
    m.def("do_allocate_test", [=]() {
        return do_allocate_v2(do_allocate_v1);
    });
    
}