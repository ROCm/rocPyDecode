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

#pragma once

#include "roc_pyjpeg_codestream.h"

namespace py = pybind11;
using namespace py::literals;

class DecodeSource {
  public:
    DecodeSource(std::unique_ptr<CodeStream> code_stream);
    DecodeSource(const CodeStream* code_stream_ptr);
    ~DecodeSource();

    DecodeSource(DecodeSource&&) = default;
    DecodeSource& operator=(DecodeSource&&) = default;

    DecodeSource(const DecodeSource&) = delete;
    DecodeSource& operator=(DecodeSource const&) = delete;

    const CodeStream* CodeStreamInstance() const;

    static void ExportToPython(py::module& m);

  private:
    std::unique_ptr<CodeStream> code_stream_;       // owned by this instance
    const CodeStream* code_stream_ptr_ = nullptr;   // externally provided
};
