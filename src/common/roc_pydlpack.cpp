/*
Copyright (c) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

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

#include <algorithm>
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;
#include "roc_pydlpack.h"
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

namespace {
void ReleaseTensorMetadata(DLManagedTensor *self) {
    delete[] self->dl_tensor.shape;
    self->dl_tensor.shape = nullptr;
    delete[] self->dl_tensor.strides;
    self->dl_tensor.strides = nullptr;
}

template <typename Target, typename Source>
Target CheckedNumericCast(Source value, const char *context) {
    if constexpr (std::is_signed_v<Source> && std::is_signed_v<Target>) {
        if (value < static_cast<Source>(std::numeric_limits<Target>::min()) ||
            value > static_cast<Source>(std::numeric_limits<Target>::max())) {
            throw std::runtime_error(std::string(context) + " is out of range");
        }
    } else if constexpr (std::is_signed_v<Source> && !std::is_signed_v<Target>) {
        using UnsignedSource = std::make_unsigned_t<Source>;
        if (value < 0 ||
            static_cast<UnsignedSource>(value) > std::numeric_limits<Target>::max()) {
            throw std::runtime_error(std::string(context) + " is out of range");
        }
    } else if constexpr (!std::is_signed_v<Source> && std::is_signed_v<Target>) {
        using UnsignedTarget = std::make_unsigned_t<Target>;
        if (value > static_cast<UnsignedTarget>(std::numeric_limits<Target>::max())) {
            throw std::runtime_error(std::string(context) + " is out of range");
        }
    } else if (value > std::numeric_limits<Target>::max()) {
        throw std::runtime_error(std::string(context) + " is out of range");
    }
    return static_cast<Target>(value);
}

DLManagedTensor MakeManagedTensor(const DLTensor &tensor) {
    DLManagedTensor managed_tensor{};
    managed_tensor.dl_tensor = tensor;
    return managed_tensor;
}
} // namespace

DLPackPyTensor::DLPackPyTensor() noexcept : m_tensor{} {
    m_tensor.deleter = ReleaseTensorMetadata;
}

DLPackPyTensor::DLPackPyTensor(DLManagedTensor &&managedTensor) : m_tensor{std::move(managedTensor)} {
    managedTensor = {};
}

DLPackPyTensor::DLPackPyTensor(const DLTensor &tensor) : DLPackPyTensor(MakeManagedTensor(tensor)) {
}

DLPackPyTensor::DLPackPyTensor(const py::buffer_info &info, const DLDevice &dev) : m_tensor{} {
    DLTensor &dlTensor = m_tensor.dl_tensor;
    const auto rank = CheckedNumericCast<size_t>(info.ndim, "tensor rank");
    if (info.shape.size() != rank || info.strides.size() != rank) {
        throw std::runtime_error("Buffer shape and stride rank must match tensor rank");
    }
    if (info.itemsize <= 0) {
        throw std::runtime_error("Buffer item size must be positive");
    }
    dlTensor.data      = info.ptr;
    //TBD dtype
    dlTensor.dtype.code = kDLInt;
    dlTensor.dtype.bits = 8;
    dlTensor.dtype.lanes = 1;
    dlTensor.ndim        = CheckedNumericCast<int32_t>(info.ndim, "tensor rank");
    dlTensor.device      = dev;
    dlTensor.byte_offset = 0;

    m_tensor.deleter = ReleaseTensorMetadata;

    try {
        auto shape = std::make_unique<int64_t[]>(rank);
        auto strides = std::make_unique<int64_t[]>(rank);
        for (size_t i = 0; i < rank; ++i) {
            shape[i] = CheckedNumericCast<int64_t>(info.shape[i], "shape dimension");
            const auto stride = info.strides[i];
            if (stride % info.itemsize != 0) {
                throw std::runtime_error("Stride must be a multiple of the element size in bytes");
            }
            strides[i] = CheckedNumericCast<int64_t>(stride / info.itemsize, "stride element");
        }

        dlTensor.shape = shape.release();
        dlTensor.strides = strides.release();
    } catch (...) {
        m_tensor.deleter(&m_tensor);
        throw;
    }
}

DLPackPyTensor::DLPackPyTensor(DLPackPyTensor &&that) noexcept : m_tensor{std::move(that.m_tensor)} {
    that.m_tensor = {};
}

DLPackPyTensor::~DLPackPyTensor() {
    if (m_tensor.deleter) {
        m_tensor.deleter(&m_tensor);
    }
}

DLPackPyTensor &DLPackPyTensor::operator=(DLPackPyTensor &&that) noexcept {
    if (this != &that) {
        if (m_tensor.deleter) {
            m_tensor.deleter(&m_tensor);
        }
        m_tensor = std::move(that.m_tensor);
        that.m_tensor = {};
    }
    return *this;
}

const DLTensor *DLPackPyTensor::operator->() const {
    return &m_tensor.dl_tensor;
}

DLTensor *DLPackPyTensor::operator->() {
    return &m_tensor.dl_tensor;
}

const DLTensor &DLPackPyTensor::operator*() const {
    return m_tensor.dl_tensor;
}

DLTensor &DLPackPyTensor::operator*() {
    return m_tensor.dl_tensor;
}

void DLPackPyTensor::test_all() {
    std::cout << "Running DLPackPyTensor::test_all()" << std::endl;
    std::vector<ssize_t> shape = {2, 3};
    std::vector<ssize_t> strides = {3, 1};
    std::vector<uint8_t> buffer(6, 42);
    py::buffer_info info(buffer.data(), sizeof(uint8_t), py::format_descriptor<uint8_t>::format(), 2, shape, strides);
    DLDevice dev;
    dev.device_type = kDLCPU;
    dev.device_id = 0;
    DLPackPyTensor tensor1(info, dev);
    std::cout << "Created tensor1 from buffer_info." << std::endl;
    DLPackPyTensor tensor2(std::move(tensor1));
    std::cout << "Move constructed tensor2 from tensor1." << std::endl;
    DLPackPyTensor tensor3;
    tensor3 = std::move(tensor2);
    std::cout << "Move assigned tensor3 from tensor2." << std::endl;
    DLTensor* raw_ptr = tensor3.operator->();
    std::cout << "Accessed raw DLTensor* via operator->, ndim = " << raw_ptr->ndim << std::endl;
    DLTensor& ref = *tensor3;
    std::cout << "Accessed DLTensor& via operator*, ndim = " << ref.ndim << std::endl;
}
