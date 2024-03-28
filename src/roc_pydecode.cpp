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

#include "roc_pyvideodecode.h"

using namespace std;

PYBIND11_MODULE(rocPyDecode, m) {
 
    m.doc() = "Python bindings for the C++ portions of rocDecode ..";

    // convert betweeen demuxer & decoder
    m.def("AVCodec2RocDecVideoCodec", &ConvertAVCodec2RocDecVideoCodec, "Convert AVCodecID to rocDecVideoCodec ID");

    // ------
    // Types:
    // ------
    py::module types_m = m.def_submodule("decTypes");
    types_m.doc() = "Datatypes and options used by rocDecode";            
        
    // rocDecVideoSurfaceFormat
    py::enum_<rocDecVideoSurfaceFormat>(types_m, "rocDecVideoSurfaceFormat")
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
        .value("rocDecVideoCodec_AVC",rocDecVideoCodec_AVC)            
        .value("rocDecVideoCodec_HEVC",rocDecVideoCodec_HEVC)          
        .export_values(); 
        
    // -------------------------------
    // USER Demuxer 'PyVideoDemuxer'
    // -------------------------------
    pyVideoDemuxerInitializer(m);

    // --------------------------------------
    // AMD Video Decoder 'PyRocVideoDecoder'
    // --------------------------------------
    pyRocVideoDecoderInitializer(m);

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
        .def_readwrite("left",&Rect::l)
        .def_readwrite("top",&Rect::t)
        .def_readwrite("right",&Rect::r)
        .def_readwrite("bottom",&Rect::b);

    // PacketData
    py::class_<PacketData, shared_ptr<PacketData>>(m, "PacketData", py::module_local())
        .def(py::init<>())
        .def_readwrite("end_of_stream", &PacketData::end_of_stream)
        .def_readwrite("pkt_flags",     &PacketData::pkt_flags)
        .def_readwrite("frame_pts",     &PacketData::frame_pts)
        .def_readwrite("frame_size",    &PacketData::frame_size)
        .def_readwrite("frame_adrs",    &PacketData::frame_adrs);   

    // ConfigInfo
    py::class_<ConfigInfo, shared_ptr<ConfigInfo>>(m, "ConfigInfo", py::module_local())
        .def(py::init<>())
        .def_readwrite("device_name",   &ConfigInfo::device_name)
        .def_readwrite("gcn_arch_name", &ConfigInfo::gcn_arch_name)
        .def_readwrite("pci_bus_id",    &ConfigInfo::pci_bus_id)
        .def_readwrite("pci_domain_id", &ConfigInfo::pci_domain_id)
        .def_readwrite("pci_device_id", &ConfigInfo::pci_device_id);            
}