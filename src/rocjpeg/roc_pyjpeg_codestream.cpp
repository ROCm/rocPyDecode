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

#include "roc_pyjpeg_decoder.h"
#include "common/roc_pybuffer.h"
#include "roc_pyjpeg_codestream.h"
#include <algorithm>
#include <vector>
#include <functional>
#include <utility>

using namespace std;
using namespace py::literals;

void CodeStream::ExportToPython(py::module& m) {
    py::class_<CodeStream>(m, "CodeStream",
        R"pbdoc(
        Class representing a coded stream of image data.

        This class provides access to image information such as dimensions.
        It supports initialization from bytes, numpy arrays, or file path.
        )pbdoc")
        .def(py::init([](py::bytes bytes) {
            return new CodeStream(bytes);
            }),
            "bytes"_a,
            R"pbdoc(
            Initialize a CodeStream using bytes as input.

            Args:
                bytes: The byte data representing the encoded stream.
            )pbdoc")
        .def(py::init([](py::array_t<uint8_t> arr) {
            return new CodeStream(arr);
            }),
            "array"_a,
            R"pbdoc(
            Initialize a CodeStream using a numpy array of uint8 as input.

            Args:
                array: The numpy array containing the encoded stream.
            )pbdoc")
        .def(py::init([](const std::filesystem::path& filename) {
            return new CodeStream(filename);
            }),
            "filename"_a,
            R"pbdoc(
            Initialize a CodeStream using a file path as input.

            Args:
                filename: The file path to the encoded stream data.
            )pbdoc");
}

int CodeStream::ReadFromFile(const std::filesystem::path& filename, std::shared_ptr<std::vector<char>>& file_buffer, size_t& file_size) {
    // Open image file in binary mode and go to the end to get file size
    std::ifstream input(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        std::cerr << "ERROR: Cannot open image: " << filename << std::endl;
        return EXIT_FAILURE;
    }
    // Get the size
    const auto raw_file_size = input.tellg();
    if (raw_file_size <= 0) {
        std::cerr << "ERROR: Invalid image size: " << filename << std::endl;
        return EXIT_FAILURE;
    }
    file_size = static_cast<size_t>(raw_file_size);
    input.seekg(0, std::ios::beg);
    // Allocate shared buffer if not already allocated or too small
    if (!file_buffer || file_buffer->size() < file_size) {
        file_buffer = std::make_shared<std::vector<char>>(file_size);
    }
    // Read the file into the buffer
    if (!input.read(file_buffer->data(), static_cast<std::streamsize>(file_size))) {
        std::cerr << "ERROR: Cannot read from file: " << filename << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// Use the dat and its size if valid, otherwise use the file to load the data
int CodeStream::InitializeSingleImage(const std::filesystem::path& filename, const unsigned char* data, size_t data_size) {
    // File sanity check
    if(!filename.empty()) {
        if(!std::filesystem::exists(filename)) {
            std::cerr << "Invalid or missing file: " << filename << std::endl;
            return EXIT_FAILURE;
        }
    }
    // Read file data, if no data sent
    if (data != nullptr && data_size > 0) {
        auto buffer = std::make_shared<std::vector<char>>(data_size);
        std::copy_n(reinterpret_cast<const char *>(data), data_size, buffer->begin());
        file_data = std::move(buffer);
    } else if(data == nullptr) {
        if (ReadFromFile(filename, file_data, data_size) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    return InitializeStreamFromCurrentData();
}

int CodeStream::InitializeStreamFromCurrentData() {
    if (!file_data || file_data->empty()) {
        std::cerr << "ERROR: Empty JPEG stream" << std::endl;
        return EXIT_FAILURE;
    }

    RocJpegStatus rocjpeg_status = ROCJPEG_STATUS_NOT_INITIALIZED;
    rocjpeg_status = rocJpegStreamCreate(&stream_handle);
    if (rocjpeg_status != ROCJPEG_STATUS_SUCCESS) {
        std::cerr << "ERROR: Failed to create stream with " << rocJpegGetErrorName(rocjpeg_status) << std::endl;
        return EXIT_FAILURE;
    }
    // Stream Parse
    rocjpeg_status = rocJpegStreamParse(reinterpret_cast<unsigned char*>(file_data->data()), file_data->size(), stream_handle);
    if (rocjpeg_status != ROCJPEG_STATUS_SUCCESS) {
        std::cerr << "ERROR: Failed to parse the input jpeg stream with " << rocJpegGetErrorName(rocjpeg_status) << std::endl;
        Release();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void CodeStream::Release() {
    if(stream_handle) {
        rocJpegStreamDestroy(stream_handle);
        stream_handle = nullptr;
    }
}

CodeStream::CodeStream(const CodeStream& other)
    : file_data(other.file_data),
      data_ref_bytes_(other.data_ref_bytes_),
      data_ref_arr_(other.data_ref_arr_) {
    if (other.stream_handle != nullptr) {
        InitializeStreamFromCurrentData();
    }
}

CodeStream& CodeStream::operator=(const CodeStream& other) {
    if (this == &other) {
        return *this;
    }

    Release();
    file_data = other.file_data;
    data_ref_bytes_ = other.data_ref_bytes_;
    data_ref_arr_ = other.data_ref_arr_;

    if (other.stream_handle != nullptr) {
        InitializeStreamFromCurrentData();
    }
    return *this;
}

CodeStream::CodeStream(CodeStream&& other) noexcept
    : stream_handle(std::exchange(other.stream_handle, nullptr)),
      file_data(std::move(other.file_data)),
      data_ref_bytes_(std::move(other.data_ref_bytes_)),
      data_ref_arr_(std::move(other.data_ref_arr_)) {}

CodeStream& CodeStream::operator=(CodeStream&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    Release();
    stream_handle = std::exchange(other.stream_handle, nullptr);
    file_data = std::move(other.file_data);
    data_ref_bytes_ = std::move(other.data_ref_bytes_);
    data_ref_arr_ = std::move(other.data_ref_arr_);
    return *this;
}

CodeStream::CodeStream(const std::filesystem::path& filename) {
    py::gil_scoped_release release;
    InitializeSingleImage(filename, nullptr, 0);
}

CodeStream::CodeStream(const unsigned char* data, size_t length) {
    py::gil_scoped_release release;
    InitializeSingleImage({}, data, length);
}

CodeStream::CodeStream(py::bytes data) {
    data_ref_bytes_ = data;
    std::string data_str = static_cast<std::string>(data_ref_bytes_); // Convert py::bytes to std::string
    std::string_view data_view(data_str);
    py::gil_scoped_release release;
    InitializeSingleImage({}, reinterpret_cast<const unsigned char*>(data_view.data()), data_view.size());
}

CodeStream::CodeStream(py::array_t<uint8_t> arr) {
    data_ref_arr_ = arr;
    auto data = data_ref_arr_.unchecked<1>();
    py::gil_scoped_release release;
    InitializeSingleImage({}, data.data(0), static_cast<size_t>(data.size()));
}

CodeStream::CodeStream() {
}

CodeStream::~CodeStream() {
    Release();
}
