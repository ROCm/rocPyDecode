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
#include "roc_pyjpeg_decode_source.h"

DecodeSource::DecodeSource(const CodeStream* code_stream_ptr)
    : code_stream_(std::make_unique<CodeStream>(*code_stream_ptr))  // make a copy
    , code_stream_ptr_(code_stream_.get()) {
}

DecodeSource::DecodeSource(std::unique_ptr<CodeStream> code_stream)
    : code_stream_(std::move(code_stream))
    , code_stream_ptr_(code_stream_.get()) {
}

const CodeStream* DecodeSource::CodeStreamInstance() const {
    return code_stream_ptr_;
}

DecodeSource::~DecodeSource() {
}

void DecodeSource::ExportToPython(py::module& m) {
    py::class_<DecodeSource>(m, "DecodeSource",
        "Class representing a source for decoding, which includes the image code stream to decode.")        
        .def(py::init([](const CodeStream* code_stream) {
                return new DecodeSource(code_stream);
            }),
            "Constructor initializing DecodeSource with a code stream of the image to decode.",
            "code_stream"_a)
        .def(py::init([](const py::array_t<uint8_t> arr) {
                return new DecodeSource(std::make_unique<CodeStream>(arr));
            }),
            "Constructor initializing DecodeSource with a numpy array.",
            "array"_a)
        .def(py::init([](const py::bytes bytes) {
                return new DecodeSource(std::make_unique<CodeStream>(bytes));
            }),
            "Constructor initializing DecodeSource with byte data.",
            "bytes"_a)
        .def(py::init([](const std::string& filename) {
                return new DecodeSource(std::make_unique<CodeStream>(std::filesystem::path(filename)));
            }),
            "Constructor initializing DecodeSource with filename pointing to the file with image.",
            "filename"_a)
        .def_property_readonly("code_stream", &DecodeSource::CodeStreamInstance,
            "Returns the code stream to be decoded into an image.");
    py::implicitly_convertible<py::bytes, DecodeSource>();
    py::implicitly_convertible<py::array_t<uint8_t>, DecodeSource>();
    py::implicitly_convertible<std::string, DecodeSource>();
    py::implicitly_convertible<py::tuple, DecodeSource>();
    py::implicitly_convertible<CodeStream, DecodeSource>();
}
