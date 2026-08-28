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

#include "roc_pybuffer.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

using namespace std;
using namespace py::literals;

namespace {
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

void ReleaseTensorShapeAndStrides(DLTensor &tensor) {
    delete[] tensor.shape;
    tensor.shape = nullptr;
    delete[] tensor.strides;
    tensor.strides = nullptr;
}

std::unique_ptr<int64_t[]> MakeTensorMetadataArray(const std::vector<size_t> &values, const char *context) {
    auto data = std::make_unique<int64_t[]>(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        data[i] = CheckedNumericCast<int64_t>(values[i], context);
    }
    return data;
}
} // namespace

static void CheckValidBuffer(const void *ptr) {
    if (ptr == nullptr) {
        throw std::runtime_error("NULL buffer not accepted");
    }
}

BufferInterface::BufferInterface(DLPackPyTensor &&dlTensor) {
    if (dlTensor->data != nullptr) {
        CheckValidBuffer(dlTensor->data);
    }
    m_dlTensor = std::move(dlTensor);
}

py::tuple BufferInterface::shape() const {
    const auto ndim = static_cast<size_t>(m_dlTensor->ndim);
    py::tuple shape(ndim);
    if (m_dlTensor->shape == nullptr) {
        if (ndim == 0) {
            return shape;
        }
        throw std::runtime_error("DLPack tensor shape is null");
    }

    std::vector<int64_t> values(ndim);
    std::copy_n(m_dlTensor->shape, ndim, values.begin());
    for (size_t i = 0; i < ndim; ++i) {
        shape[i] = values[i];
    }
    return shape;
}

py::tuple BufferInterface::strides() const {
    const auto ndim = static_cast<size_t>(m_dlTensor->ndim);
    py::tuple strides(ndim);
    if (m_dlTensor->strides == nullptr) {
        if (ndim == 0) {
            return strides;
        }
        if (m_dlTensor->shape == nullptr) {
            throw std::runtime_error("Cannot compute contiguous strides without a tensor shape");
        }

        int64_t current_stride = 1;
        for (size_t i = ndim; i-- > 0;) {
            const int64_t dimension = m_dlTensor->shape[i];
            if (dimension < 0 ||
                (dimension != 0 && current_stride > std::numeric_limits<int64_t>::max() / dimension)) {
                throw std::runtime_error("Invalid tensor shape while computing contiguous strides");
            }
            strides[i] = current_stride;
            current_stride *= dimension;
        }
        return strides;
    }

    std::vector<int64_t> values(ndim);
    std::copy_n(m_dlTensor->strides, ndim, values.begin());
    for (size_t i = 0; i < ndim; ++i) {
        strides[i] = values[i];
    }
    return strides;
}

std::string BufferInterface::dtype() const {
    if (m_dlTensor->dtype.bits == 8)
        return std::string("|u1");
    else if (m_dlTensor->dtype.bits == 16)
        return std::string("|u2");
    return std::string("|u1"); // non-void function must ret value
}

void *BufferInterface::data() const {
    return m_dlTensor->data;
}

py::capsule BufferInterface::dlpack(py::object stream) const {
    static_cast<void>(stream);

    struct ManagerCtx {
        DLManagedTensor tensor;
        std::shared_ptr<const BufferInterface> extBuffer;
    };

    auto ctx = std::make_unique<ManagerCtx>();

    // Set up tensor deleter to delete the ManagerCtx
    ctx->tensor.manager_ctx = ctx.get();
    ctx->tensor.deleter = [](DLManagedTensor *tensor) {
        delete static_cast<ManagerCtx *>(tensor->manager_ctx);
    };

    // Copy tensor data
    ctx->tensor.dl_tensor = *m_dlTensor;

    // Manager context holds a reference to this External Buffer so that
    // GC doesn't delete this buffer while the dlpack tensor still refers to it.
    ctx->extBuffer = this->shared_from_this();

    // Creates the python capsule with the DLManagedTensor instance we're returning.
    py::capsule cap(&ctx->tensor, "dltensor", [](PyObject *ptr) {
                        if(PyCapsule_IsValid(ptr, "dltensor")) {
                            // If consumer didn't delete the tensor,
                            if(auto *dlTensor = static_cast<DLManagedTensor *>(PyCapsule_GetPointer(ptr, "dltensor"))) {
                                // Delete the tensor.
                                if(dlTensor->deleter != nullptr) {
                                    dlTensor->deleter(dlTensor);
                                }
                            }
                        } });

    // Now that the capsule is created and the manager ctx was transfered to it,
    // we can release the unique_ptr.
    ctx.release();

    return cap;
}

py::tuple BufferInterface::dlpackDevice() const {
    return py::make_tuple(py::int_(static_cast<int>(m_dlTensor->device.device_type)),
                          py::int_(static_cast<int>(m_dlTensor->device.device_id)));
}

const DLTensor &BufferInterface::dlTensor() const {
    return *m_dlTensor;
}

void BufferInterface::ExportToPython(py::module &m) {
    py::class_<BufferInterface, std::shared_ptr<BufferInterface>>(m, "BufferInterface", py::dynamic_attr())
        .def_property_readonly("shape", &BufferInterface::shape, "Get the shape of the buffer as an array")
        .def_property_readonly("strides", &BufferInterface::strides, "Get the strides of the buffer")
        .def_property_readonly("dtype", &BufferInterface::dtype, "Get the data type of the buffer")
        .def("__dlpack__", &BufferInterface::dlpack, "stream"_a=1, "Export the buffer as a DLPack tensor")
        .def("__dlpack_device__", &BufferInterface::dlpackDevice, "Get the device associated with the buffer");
}

int BufferInterface::LoadDLPack(const std::vector<size_t>& _shape, const std::vector<size_t>& _stride, uint32_t bit_depth, const std::string& _type_str, void* _data, int device_id_) {
    if (_shape.size() != _stride.size()) {
        throw std::runtime_error("Shape and stride rank must match");
    }

    m_dlTensor->byte_offset = 0;
    m_dlTensor->device.device_type = kDLROCM;   // TODO: infer the device type from the memory buffer
    m_dlTensor->device.device_id = device_id_;

    // Convert data
    void* ptr = _data;
    CheckValidBuffer(ptr);
    m_dlTensor->data = ptr;

    // Convert DataType
    if (_type_str != "|u1" && _type_str != "|u2") {  // TODO: can also be other letters
        throw std::runtime_error("Could not create DL Pack tensor! Invalid typstr: " + _type_str);
    }

    m_dlTensor->dtype.code = kDLUInt;
    int item_size_dt = 0;
    if (bit_depth == 8U) {
        m_dlTensor->dtype.bits = 8U;
        item_size_dt = static_cast<int>(sizeof(uint8_t));
    } else if (bit_depth == 10U) {
        m_dlTensor->dtype.bits = 16U;
        item_size_dt = static_cast<int>(sizeof(uint16_t));
    } else {
        throw std::runtime_error("Unsupported bit depth for DLPack export");
    }
    m_dlTensor->dtype.lanes = 1;

    // Prepare replacement metadata before modifying the current tensor so an
    // allocation or conversion failure leaves the existing metadata intact.
    const auto ndim = CheckedNumericCast<int>(_shape.size(), "tensor rank");
    auto shape = MakeTensorMetadataArray(_shape, "shape dimension");
    auto strides = std::make_unique<int64_t[]>(_stride.size());
    for (size_t i = 0; i < _stride.size(); ++i) {
        const auto stride_bytes = CheckedNumericCast<int64_t>(_stride[i], "stride");
        if (stride_bytes % item_size_dt != 0) {
            throw std::runtime_error("Stride must be a multiple of the element size in bytes");
        }
        strides[i] = stride_bytes / item_size_dt;
    }

    ReleaseTensorShapeAndStrides(*m_dlTensor);
    m_dlTensor->ndim = ndim;
    m_dlTensor->shape = shape.release();
    m_dlTensor->strides = strides.release();
    return 0;
}
