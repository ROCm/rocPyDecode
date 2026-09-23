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

#include "roc_pybuffer.h"
#include <limits>
#include "roc_pyvideodecode.h"

using namespace std;
#ifndef NDEBUG
void TestAllClassCalls(const char*);
void TestAll_roc_pybuffer();
void Test_DLPackPyTensor_ConstructorsAndOperators();
void Test_PyReconfigureFlushCallback(const char*, const std::string&);
void Test_CalculateRgbImageSize();
#endif


PYBIND11_MODULE(rocpydecode, m) {
 
    m.doc() = "Python bindings for the C++ portions of rocDecode ..";

#ifndef NDEBUG
    m.def("TestAllClassCalls", &TestAllClassCalls);
    m.def("TestAll_roc_pybuffer", &TestAll_roc_pybuffer);
    m.def("Test_DLPack", &Test_DLPackPyTensor_ConstructorsAndOperators);
    m.def("Test_PyReconfigureFlushCallback", &Test_PyReconfigureFlushCallback);
    m.def("Test_CalculateRgbImageSize", &Test_CalculateRgbImageSize);
#endif
    m.def("GetRocPyDecPacket", [](int64_t pts, int64_t size, py::buffer buffer) {
        auto info = buffer.request();
        if (size < 0 || size > std::numeric_limits<uint32_t>::max() ||
            size > info.size * info.itemsize)
            throw std::invalid_argument("Packet size exceeds its buffer or decoder limit");
        py::ssize_t stride = info.itemsize;
        for (py::ssize_t i = info.ndim; i-- > 0;) {
            if (info.shape[static_cast<size_t>(i)] > 1 && info.strides[static_cast<size_t>(i)] != stride)
                throw std::invalid_argument("Packet buffer must be C-contiguous");
            stride *= info.shape[static_cast<size_t>(i)];
        }
        auto packet = make_shared<PyPacketData>();
        packet->frame_pts = pts;
        packet->pkt_flags = ROCDEC_PKT_TIMESTAMP;
        packet->bitstream_size = size;
        packet->bitstream_adrs = size ? reinterpret_cast<uintptr_t>(info.ptr) : 0;
        packet->end_of_stream = size == 0;
        auto result = py::cast(packet);
        // Hold an exported view, not just the exporter: a bytearray must not
        // resize and invalidate the address while this packet is alive.
        result.attr("_buffer_owner") = py::memoryview(buffer);
        return result;
    }, "Wrap a contiguous packet buffer and retain its exported view");

    // Resolve Python API classes and codec helpers on demand.
    m.def("__getattr__", [](const std::string& name) -> py::object {
        if (name == "PyVideoDemuxer" || name == "PyFileStreamProvider") {
            auto module = py::module_::import("pyRocVideoDecode.demuxer");
            return module.attr(name == "PyVideoDemuxer" ? "demuxer" : "stream_provider");
        }
        if (name == "PyRocVideoDecoderCpu")
            return py::module_::import("pyRocVideoDecode.decodercpu").attr("PyRocVideoDecoderCpu");
        if (name == "AVCodec2RocDecVideoCodec" || name == "AVCodecString2RocDecVideoCodec")
            return py::module_::import("pyRocVideoDecode._pyav").attr("codec_id");
        throw py::attribute_error("module rocpydecode has no attribute " + name);
    });

    // ------
    // Types:
    // ------
    py::module types_m = m.def_submodule("decTypes");
    types_m.doc() = "Datatypes and options used by rocDecode";            

    // current version
    // Todo: to be changed to match version on CMakeLists with every future version update
    m.attr("__version__") = py::str("1.0.0");

    // OutputSurfaceMemoryType
    py::enum_<OutputSurfaceMemoryType>(types_m, "OutputSurfaceMemoryType")
        .value("OUT_SURFACE_MEM_DEV_INTERNAL",OUT_SURFACE_MEM_DEV_INTERNAL)
        .value("OUT_SURFACE_MEM_DEV_COPIED",OUT_SURFACE_MEM_DEV_COPIED)
        .value("OUT_SURFACE_MEM_HOST_COPIED",OUT_SURFACE_MEM_HOST_COPIED)
        .value("OUT_SURFACE_MEM_NOT_MAPPED",OUT_SURFACE_MEM_NOT_MAPPED)
        .export_values();

    // plain int constants
    types_m.attr("OUT_SURFACE_MEM_DEV_INTERNAL") = py::int_(static_cast<int>(OUT_SURFACE_MEM_DEV_INTERNAL));
    types_m.attr("OUT_SURFACE_MEM_DEV_COPIED")   = py::int_(static_cast<int>(OUT_SURFACE_MEM_DEV_COPIED));
    types_m.attr("OUT_SURFACE_MEM_HOST_COPIED")  = py::int_(static_cast<int>(OUT_SURFACE_MEM_HOST_COPIED));
    types_m.attr("OUT_SURFACE_MEM_NOT_MAPPED")   = py::int_(static_cast<int>(OUT_SURFACE_MEM_NOT_MAPPED));

    // rocDecVideoSurfaceFormat
    py::enum_<rocDecVideoSurfaceFormat>(types_m, "rocDecVideoSurfaceFormat")
        .value("rocDecVideoSurfaceFormat_YUV420",rocDecVideoSurfaceFormat_YUV420)
        .value("rocDecVideoSurfaceFormat_YUV420_16Bit",rocDecVideoSurfaceFormat_YUV420_16Bit)
        .value("rocDecVideoSurfaceFormat_YUV422",rocDecVideoSurfaceFormat_YUV422)
        .value("rocDecVideoSurfaceFormat_YUV422_16Bit",rocDecVideoSurfaceFormat_YUV422_16Bit)
        .value("rocDecVideoSurfaceFormat_NV12",rocDecVideoSurfaceFormat_NV12)					
        .value("rocDecVideoSurfaceFormat_P016",rocDecVideoSurfaceFormat_P016)					
        .value("rocDecVideoSurfaceFormat_YUV444",rocDecVideoSurfaceFormat_YUV444)				
        .value("rocDecVideoSurfaceFormat_YUV444_16Bit",rocDecVideoSurfaceFormat_YUV444_16Bit) 	
		.export_values(); 
                
    py::enum_<RocdecVideoPacketFlags>(types_m,"RocdecVideoPacketFlags","Video Packet Flags")
        .value("ROCDEC_PKT_ENDOFSTREAM",ROCDEC_PKT_ENDOFSTREAM)
        .value("ROCDEC_PKT_TIMESTAMP",ROCDEC_PKT_TIMESTAMP)
        .value("ROCDEC_PKT_DISCONTINUITY",ROCDEC_PKT_DISCONTINUITY)
        .value("ROCDEC_PKT_ENDOFPICTURE",ROCDEC_PKT_ENDOFPICTURE)
        .value("ROCDEC_PKT_NOTIFY_EOS",ROCDEC_PKT_NOTIFY_EOS)
        .export_values(); 

    py::enum_<rocDecVideoCodec>(types_m,"rocDecVideoCodec","Video Codec") 
        .value("rocDecVideoCodec_MPEG1",rocDecVideoCodec_MPEG1)
        .value("rocDecVideoCodec_MPEG2",rocDecVideoCodec_MPEG2)
        .value("rocDecVideoCodec_MPEG4",rocDecVideoCodec_MPEG4)
        .value("rocDecVideoCodec_JPEG",rocDecVideoCodec_JPEG)
        .value("rocDecVideoCodec_VP8",rocDecVideoCodec_VP8)
        .value("rocDecVideoCodec_VP9",rocDecVideoCodec_VP9)
        .value("rocDecVideoCodec_AV1",rocDecVideoCodec_AV1)
        .value("rocDecVideoCodec_AVC",rocDecVideoCodec_AVC)            
        .value("rocDecVideoCodec_HEVC",rocDecVideoCodec_HEVC)          
        .export_values(); 

    py::enum_<OutputFormatEnum>(types_m,"OutputFormatEnum","Types of images")
        .value("native",native)
        .value("bgr",bgr)
        .value("bgr48",bgr48)
        .value("rgb",rgb)
        .value("rgb48",rgb48)
        .value("bgra",bgra)
        .value("bgra64",bgra64)
        .value("rgba",rgba)
        .value("rgba64",rgba64)
       .export_values();

    // ---------
    // PyExport
    // ---------
    BufferInterface::ExportToPython(m);

    PyRocVideoDecoderInitializer(m);

    // ----------------
    // Structures:
    // ----------------

    // OutputSurfaceInfo
    py::class_<OutputSurfaceInfo>(m, "OutputSurfaceInfo")
        .def(py::init<>())
        .def_readwrite("output_width",&OutputSurfaceInfo::output_width)	            	            	  
        .def_readwrite("output_height",&OutputSurfaceInfo::output_height)		            	          
        .def_readwrite("output_pitch",&OutputSurfaceInfo::output_pitch)	     	            	                             
        .def_readwrite("output_vstride",&OutputSurfaceInfo::output_vstride)	   	            	          
        .def_readwrite("bytes_per_pixel",&OutputSurfaceInfo::bytes_per_pixel)		            	      
        .def_readwrite("bit_depth",&OutputSurfaceInfo::bit_depth)	           	            	          
        .def_readwrite("num_chroma_planes",&OutputSurfaceInfo::num_chroma_planes)	            	      
        .def_readwrite("output_surface_size_in_bytes",&OutputSurfaceInfo::output_surface_size_in_bytes)   
        .def_readwrite("surface_format",&OutputSurfaceInfo::surface_format)			            	      
        .def_readwrite("mem_type",&OutputSurfaceInfo::mem_type);                                                  				
  
    // Rect
    py::class_<Rect>(m, "Rect")
        .def(py::init<>())
        .def_readwrite("left",&Rect::left)
        .def_readwrite("top",&Rect::top)
        .def_readwrite("right",&Rect::right)
        .def_readwrite("bottom",&Rect::bottom);

    // Dim
    py::class_<Dim>(m, "Dim")
        .def(py::init<>())
        .def_readwrite("width",&Dim::w)
        .def_readwrite("height",&Dim::h);

    // PyPacketData
    py::class_<PyPacketData, shared_ptr<PyPacketData>>(m, "PyPacketData", py::module_local(), py::dynamic_attr())
        .def(py::init<>())
        .def_readwrite("end_of_stream", &PyPacketData::end_of_stream)
        .def_readwrite("pkt_flags",     &PyPacketData::pkt_flags)
        .def_readwrite("frame_pts",     &PyPacketData::frame_pts)
        .def_readwrite("frame_size",    &PyPacketData::frame_size)
        .def_readwrite("frame_adrs",    &PyPacketData::frame_adrs)
        .def_readwrite("bitstream_size",    &PyPacketData::bitstream_size)
        .def_readwrite("bitstream_adrs",    &PyPacketData::bitstream_adrs)
        .def_readwrite("frame_adrs_rgb", &PyPacketData::frame_adrs_rgb)
        .def_readwrite("frame_adrs_resized", &PyPacketData::frame_adrs_resized)
        .def_readwrite("ext_buf",        &PyPacketData::ext_buf)

        // DL Pack Tensor
        .def_property_readonly("shapeY", [](std::shared_ptr<PyPacketData>& self) {
            return self->ext_buf[0]->shape();
            }, "Get the shape of the Y plane buffer as an array")
        .def_property_readonly("shapeUV", [](std::shared_ptr<PyPacketData>& self) {
            return self->ext_buf[1]->shape();
            }, "Get the shape of the U plane buffer as an array")
        .def_property_readonly("shapeU", [](std::shared_ptr<PyPacketData>& self) {
            return self->ext_buf[1]->shape();
            }, "Get the shape of the U plane buffer as an array")
        .def_property_readonly("shapeV", [](std::shared_ptr<PyPacketData>& self) {
            return self->ext_buf[2]->shape();
            }, "Get the shape of the V plane buffer as an array")
        .def_property_readonly("shape", [](std::shared_ptr<PyPacketData>& self) {
            return self->ext_buf[0]->shape();
            }, "Get the shape of the buffer as an array")
        .def_property_readonly("strides", [](std::shared_ptr<PyPacketData>& self) {
                return self->ext_buf[0]->strides();
            }, "Get the strides of the buffer")
        .def_property_readonly("dtype", [](std::shared_ptr<PyPacketData>& self) {
                return self->ext_buf[0]->dtype();
            }, "Get the data type of the buffer")
        .def("__dlpack__", [](std::shared_ptr<PyPacketData>& self, py::object stream) {
            return self->ext_buf[0]->dlpack(stream);
            }, py::arg("stream") = NULL, "Export the buffer as a DLPack tensor")
        .def("__dlpack_device__", [](std::shared_ptr<PyPacketData>& self) {
                return self->ext_buf[0]->dlpackDevice();
            }, "Get the device associated with the buffer");   

    // ConfigInfo
    py::class_<ConfigInfo, shared_ptr<ConfigInfo>>(m, "ConfigInfo", py::module_local())
        .def(py::init<>())
        .def_readwrite("device_name",   &ConfigInfo::device_name)
        .def_readwrite("gcn_arch_name", &ConfigInfo::gcn_arch_name)
        .def_readwrite("pci_bus_id",    &ConfigInfo::pci_bus_id)
        .def_readwrite("pci_domain_id", &ConfigInfo::pci_domain_id)
        .def_readwrite("pci_device_id", &ConfigInfo::pci_device_id);

    PyCpuSurfaceInitializer(m);

    py::class_<DLPackPyTensor>(m, "DLPackPyTensor")
        .def(py::init<>())
        .def_static("test_all", &DLPackPyTensor::test_all);
}
