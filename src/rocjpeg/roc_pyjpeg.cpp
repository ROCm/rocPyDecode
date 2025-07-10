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

#include "roc_pyjpeg.h"
#include "common/roc_pybuffer.h"
#include "roc_pyjpeg_decoder.h"
#include "roc_pyjpeg_codestream.h"
#include "roc_pyjpeg_decode_source.h"
#include "roc_pyjpeg_images.h"

using namespace std;

namespace py = pybind11;
using namespace py::literals;

PYBIND11_MODULE(rocpyjpegdecode, m) {
 
    m.doc() = "Python bindings for the C++ portions of rocJPEG ..";

    // Types:
    py::module types_m = m.def_submodule("jpegTypes");
    types_m.doc() = R"pbdoc(

        rocPyJpegDecode Python API reference

        This is the Python API reference for the AMD ROCm rocJPEG library.
    )pbdoc";

    // current version
    // TODO: match version on CMakeLists for future version update
    m.attr("__version__") = py::str("0.6.0");

    types_m.attr("ROCJPEG_MAX_COMPONENT") = ROCJPEG_MAX_COMPONENT;

    // RocJpegChromaSubsampling
    py::enum_<RocJpegChromaSubsampling>(types_m, "RocJpegChromaSubsampling")
        .value("ROCJPEG_CSS_444",ROCJPEG_CSS_444)
        .value("ROCJPEG_CSS_440",ROCJPEG_CSS_440)
        .value("ROCJPEG_CSS_422",ROCJPEG_CSS_422)
        .value("ROCJPEG_CSS_420",ROCJPEG_CSS_420)
        .value("ROCJPEG_CSS_411",ROCJPEG_CSS_411)
        .value("ROCJPEG_CSS_400",ROCJPEG_CSS_400)
        .value("ROCJPEG_CSS_UNKNOWN",ROCJPEG_CSS_UNKNOWN)
        .export_values();

    // RocJpegBackend
    py::enum_<RocJpegBackend>(types_m,"RocJpegBackend")
        .value("ROCJPEG_BACKEND_HARDWARE",ROCJPEG_BACKEND_HARDWARE)
        .value("ROCJPEG_BACKEND_HYBRID",ROCJPEG_BACKEND_HYBRID)
        .export_values();

    // RocJpegOutputFormat
    py::enum_<RocJpegOutputFormat>(types_m, "RocJpegOutputFormat")
        .value("ROCJPEG_OUTPUT_NATIVE",ROCJPEG_OUTPUT_NATIVE)
        .value("ROCJPEG_OUTPUT_YUV_PLANAR",ROCJPEG_OUTPUT_YUV_PLANAR)
        .value("ROCJPEG_OUTPUT_Y",ROCJPEG_OUTPUT_Y)
        .value("ROCJPEG_OUTPUT_RGB",ROCJPEG_OUTPUT_RGB)
        .value("ROCJPEG_OUTPUT_RGB_PLANAR",ROCJPEG_OUTPUT_RGB_PLANAR)
        .value("ROCJPEG_OUTPUT_FORMAT_MAX",ROCJPEG_OUTPUT_FORMAT_MAX)
        .export_values();

    // RocJpegStatus
    py::enum_<RocJpegStatus>(types_m, "RocJpegStatus")
        .value("ROCJPEG_STATUS_SUCCESS",ROCJPEG_STATUS_SUCCESS)
        .value("ROCJPEG_STATUS_NOT_INITIALIZED",ROCJPEG_STATUS_NOT_INITIALIZED)
        .value("ROCJPEG_STATUS_INVALID_PARAMETER",ROCJPEG_STATUS_INVALID_PARAMETER)
        .value("ROCJPEG_STATUS_BAD_JPEG",ROCJPEG_STATUS_BAD_JPEG)
        .value("ROCJPEG_STATUS_JPEG_NOT_SUPPORTED",ROCJPEG_STATUS_JPEG_NOT_SUPPORTED)
        .value("ROCJPEG_STATUS_OUTOF_MEMORY",ROCJPEG_STATUS_OUTOF_MEMORY)
        .value("ROCJPEG_STATUS_EXECUTION_FAILED",ROCJPEG_STATUS_EXECUTION_FAILED)
        .value("ROCJPEG_STATUS_ARCH_MISMATCH",ROCJPEG_STATUS_ARCH_MISMATCH)
        .value("ROCJPEG_STATUS_INTERNAL_ERROR",ROCJPEG_STATUS_INTERNAL_ERROR)
        .value("ROCJPEG_STATUS_IMPLEMENTATION_NOT_SUPPORTED",ROCJPEG_STATUS_IMPLEMENTATION_NOT_SUPPORTED)
        .value("ROCJPEG_STATUS_HW_JPEG_DECODER_NOT_SUPPORTED",ROCJPEG_STATUS_HW_JPEG_DECODER_NOT_SUPPORTED)
        .value("ROCJPEG_STATUS_RUNTIME_ERROR",ROCJPEG_STATUS_RUNTIME_ERROR)
        .value("ROCJPEG_STATUS_NOT_IMPLEMENTED",ROCJPEG_STATUS_NOT_IMPLEMENTED)
        .export_values();

    // AMD JPEG Decoder exported classes
    Decoder::ExportToPython(m);
    CodeStream::ExportToPython(m);
    DecodeSource::ExportToPython(m);
    BufferInterface::ExportToPython(m);
    PyJpegImages::ExportToPython(m);

    // PyJpegUtils for HIP Call
    py::class_<PyRocJpegUtils>(m, "PyRocJpegUtils")
        .def(py::init<>())
        .def("init_hip_device",&PyRocJpegUtils::InitHipDevice);

}
