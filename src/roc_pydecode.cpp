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

#include "../inc/roc_pyvideodecode.h"

using namespace std;
namespace py = pybind11;

PYBIND11_MODULE(rocPyDecode, m) {
 
    m.doc() = "Python bindings for the C++ portions of rocDecode ..";

    // ----------------
    // Class/Structure:
    // ----------------

    py::class_<Rect>(m, "Rect")
        .def(py::init<>())
        .def_readwrite("l",&Rect::l)
        .def_readwrite("t",&Rect::t)
        .def_readwrite("r",&Rect::r)
        .def_readwrite("b",&Rect::b);

    // ------
    // Types:
    // ------
    py::module types_m = m.def_submodule("types");
    types_m.doc() = "Datatypes and options used by rocdecode";

    py::enum_<OutputSurfaceMemoryType>(types_m, "OutputSurfaceMemoryType", "Surface Memory Types")
        .value("OUT_SURFACE_MEM_DEV_INTERNAL",OUT_SURFACE_MEM_DEV_INTERNAL)
        .value("OUT_SURFACE_MEM_DEV_COPIED",OUT_SURFACE_MEM_DEV_COPIED)
        .value("OUT_SURFACE_MEM_HOST_COPIED",OUT_SURFACE_MEM_HOST_COPIED)
        .export_values(); 

    py::enum_<RocdecVideoPacketFlags>(types_m,"RocdecVideoPacketFlags","Video Packet Flags")
        .value("ROCDEC_PKT_ENDOFSTREAM",ROCDEC_PKT_ENDOFSTREAM)
        .value("ROCDEC_PKT_TIMESTAMP",ROCDEC_PKT_TIMESTAMP)
        .value("ROCDEC_PKT_DISCONTINUITY",ROCDEC_PKT_DISCONTINUITY)
        .value("ROCDEC_PKT_ENDOFPICTURE",ROCDEC_PKT_ENDOFPICTURE)
        .value("ROCDEC_PKT_NOTIFY_EOS",ROCDEC_PKT_NOTIFY_EOS)
        .export_values(); 

    // -------------------------------
    // USER Demuxer 'usrVideoDemuxer'
    // -------------------------------
    py::class_<pyVideoDemuxer, usrVideoDemuxer, std::shared_ptr<pyVideoDemuxer>> (m, "usrVideoDemuxer")
        .def(py::init<const char*>())
        .def("GetCodec_ID",&pyVideoDemuxer::GetCodec_ID,"Get Codec ID")
        .def("DemuxFrame",&pyVideoDemuxer::DemuxFrame);

    // --------------------------------------
    // AMD Video Decoder 'pyRocVideoDecoder'
    // --------------------------------------
    py::class_<pyRocVideoDecoder> (m, "pyRocVideoDecoder")
        .def(py::init<const char*,int,OutputSurfaceMemoryType,bool,const Rect *,bool,int,int,uint32_t>())
        .def("GetDeviceinfo",&pyRocVideoDecoder::wrapper_GetDeviceinfo)
        .def("InitMd5",&pyRocVideoDecoder::wrapper_InitMd5)
        .def("SetReconfigParams",&pyRocVideoDecoder::wrapper_SetReconfigParams)
        .def_readwrite("demuxer",&pyRocVideoDecoder::demuxer)
        .def("DecodeFrame",&pyRocVideoDecoder::wrapper_DecodeFrame)
        .def("GetFrame",&pyRocVideoDecoder::wrapper_GetFrame)
        .def("SaveFrameToFile",&pyRocVideoDecoder::wrapper_SaveFrameToFile)
        .def("ReleaseFrame",&pyRocVideoDecoder::wrapper_ReleaseFrame)
        .def("GetOutputSurfaceInfo",&pyRocVideoDecoder::wrapper_GetOutputSurfaceInfo)
        .def("FinalizeMd5",&pyRocVideoDecoder::wrapper_FinalizeMd5)
        .def("GetNumOfFlushedFrames",&pyRocVideoDecoder::wrapper_GetNumOfFlushedFrames)
        .def("UpdateMd5ForFrame",&pyRocVideoDecoder::wrapper_UpdateMd5ForFrame);
}
