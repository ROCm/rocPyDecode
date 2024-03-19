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

    // convert betweeen demuxer & decoder
    m.def("AVCodec2RocDecVideoCodec", &AVCodec2RocDecVideoCodec, "Convert AVCodecID to rocDecVideoCodec ID");

    // ------
    // Types:
    // ------
    py::module types_m = m.def_submodule("decTypes");
    types_m.doc() = "Datatypes and options used by rocdecode";

    py::enum_<AVCodecID>(types_m, "AVCodecID")
        .value("AV_CODEC_ID_NONE",AV_CODEC_ID_NONE) 
        .value("AV_CODEC_ID_MPEG1VIDEO",AV_CODEC_ID_MPEG1VIDEO) 
        .value("AV_CODEC_ID_MPEG2VIDEO",AV_CODEC_ID_MPEG2VIDEO) 
        .value("AV_CODEC_ID_MPEG4",AV_CODEC_ID_MPEG4) 
        .value("AV_CODEC_ID_H264",AV_CODEC_ID_H264) 
        .value("AV_CODEC_ID_HEVC",AV_CODEC_ID_HEVC) 
        .value("AV_CODEC_ID_VP8",AV_CODEC_ID_VP8) 
        .value("AV_CODEC_ID_VP9",AV_CODEC_ID_VP9) 
        .value("AV_CODEC_ID_MJPEG",AV_CODEC_ID_MJPEG)
        .value("AV_CODEC_ID_AV1",AV_CODEC_ID_AV1) 
        .export_values();              
        
    // rocDecVideoSurfaceFormat
    py::enum_<rocDecVideoSurfaceFormat>(types_m, "rocDecVideoSurfaceFormat")
        .value("rocDecVideoSurfaceFormat_NV12",rocDecVideoSurfaceFormat_NV12)					// Semi-Planar YUV
        .value("rocDecVideoSurfaceFormat_P016",rocDecVideoSurfaceFormat_P016)					// 16 bit Semi-Planar YUV 
        .value("rocDecVideoSurfaceFormat_YUV444",rocDecVideoSurfaceFormat_YUV444)				// Planar YUV 
        .value("rocDecVideoSurfaceFormat_YUV444_16Bit",rocDecVideoSurfaceFormat_YUV444_16Bit) 	// 16 bit Planar YUV
		.export_values(); 
                
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

    py::enum_<rocDecVideoCodec>(types_m,"rocDecVideoCodec","Video Codec") 
        .value("rocDecVideoCodec_MPEG1",rocDecVideoCodec_MPEG1)        /**<  MPEG1 */                  
        .value("rocDecVideoCodec_MPEG2",rocDecVideoCodec_MPEG2)        /**<  MPEG2 */                  
        .value("rocDecVideoCodec_MPEG4",rocDecVideoCodec_MPEG4)        /**<  MPEG4 */                  
        .value("rocDecVideoCodec_AVC",rocDecVideoCodec_AVC)            /**<  AVC/H264 */              
        .value("rocDecVideoCodec_HEVC",rocDecVideoCodec_HEVC)          /**<  HEVC */                
        .value("rocDecVideoCodec_AV1",rocDecVideoCodec_AV1)            /**<  AV1 */              
        .value("rocDecVideoCodec_VP8",rocDecVideoCodec_VP8)            /**<  VP8 */              
        .value("rocDecVideoCodec_VP9",rocDecVideoCodec_VP9)            /**<  VP9 */              
        .value("rocDecVideoCodec_JPEG",rocDecVideoCodec_JPEG)          /**<  JPEG */                
        .value("rocDecVideoCodec_NumCodecs",rocDecVideoCodec_NumCodecs)/**<  Max codecs */                          
        // Uncompressed YUV                                                                              
        .value("rocDecVideoCodec_YUV420",rocDecVideoCodec_YUV420)      /**< Y,U,V (4:2:0)      */                    
        .value("rocDecVideoCodec_YV12",rocDecVideoCodec_YV12)          /**< Y,V,U (4:2:0)      */                
        .value("rocDecVideoCodec_NV12",rocDecVideoCodec_NV12)          /**< Y,UV  (4:2:0)      */                
        .value("rocDecVideoCodec_YUYV",rocDecVideoCodec_YUYV)          /**< YUYV/YUY2 (4:2:2)  */                 
        .value("rocDecVideoCodec_UYVY",rocDecVideoCodec_UYVY)          /**< UYVY (4:2:2)       */                
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
        .def(py::init<int,OutputSurfaceMemoryType,rocDecVideoCodec,bool,const Rect *,bool,int,int,uint32_t>(),
                    py::arg("device_id"), py::arg("out_mem_type"), py::arg("codec"),
                    py::arg("force_zero_latency"), py::arg("p_crop_rect"), py::arg("extract_user_SEI_Message"),
                    py::arg("max_width"), py::arg("max_height"), py::arg("clk_rate"))
        .def("GetDeviceinfo",&pyRocVideoDecoder::wrapper_GetDeviceinfo)
        .def("InitMd5",&pyRocVideoDecoder::wrapper_InitMd5)
        .def("SetReconfigParams",&pyRocVideoDecoder::wrapper_SetReconfigParams)
        .def("DecodeFrame",&pyRocVideoDecoder::DecodeFrame) 
        .def("GetFrameAddress",&pyRocVideoDecoder::wrapper_GetFrameAddress)
        .def("GetFrame",&pyRocVideoDecoder::GetFrame)
        .def("SaveFrameToFile",&pyRocVideoDecoder::wrapper_SaveFrameToFile)
        .def("ReleaseFrame",&pyRocVideoDecoder::wrapper_ReleaseFrame)
        .def("GetOutputSurfaceInfoAdrs",&pyRocVideoDecoder::wrapper_GetOutputSurfaceInfoAdrs)
        .def("FinalizeMd5",&pyRocVideoDecoder::wrapper_FinalizeMd5)
        .def("GetNumOfFlushedFrames",&pyRocVideoDecoder::wrapper_GetNumOfFlushedFrames)
        .def("UpdateMd5ForFrame",&pyRocVideoDecoder::wrapper_UpdateMd5ForFrame);

    // ----------------
    // Structures:
    // ----------------

    // OutputSurfaceInfo
    py::class_<OutputSurfaceInfo>(m, "OutputSurfaceInfo")
        .def(py::init<>())
        .def_readwrite("output_width",&OutputSurfaceInfo::output_width)	            /**< Output width of decoded surface*/
        .def_readwrite("output_height",&OutputSurfaceInfo::output_height)	        /**< Output height of decoded surface*/
        .def_readwrite("output_pitch",&OutputSurfaceInfo::output_pitch)	            /**< Output pitch in bytes of luma plane, chroma pitch can be inferred based on chromaFormat*/
        .def_readwrite("output_vstride",&OutputSurfaceInfo::output_vstride)	        /**< Output vertical stride in case of using internal mem pointer **/
        .def_readwrite("bytes_per_pixel",&OutputSurfaceInfo::bytes_per_pixel)	    /**< Output BytesPerPixel of decoded image*/
        .def_readwrite("bit_depth",&OutputSurfaceInfo::bit_depth)	                /**< Output BitDepth of the image*/
        .def_readwrite("num_chroma_planes",&OutputSurfaceInfo::num_chroma_planes)	/**< Output Chroma number of planes*/
        .def_readwrite("output_surface_size_in_bytes",&OutputSurfaceInfo::output_surface_size_in_bytes)/**< Output Image Size in Bytes; including both luma and chroma planes*/ 
        .def_readwrite("surface_format",&OutputSurfaceInfo::surface_format)			/**< Chroma format of the decoded image*/
        .def_readwrite("mem_type",&OutputSurfaceInfo::mem_type);					/**< Output mem_type of the surface*/  
  
    // Rect
    py::class_<Rect>(m, "Rect")
        .def(py::init<>())
        .def_readwrite("left",&Rect::left)
        .def_readwrite("top",&Rect::top)
        .def_readwrite("right",&Rect::right)
        .def_readwrite("bottom",&Rect::bottom);

}

