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

#pragma once 

#include <iostream>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

#include "rocdecode.h"
/*!
 * \file
 * \brief The AMD Video Demuxer for rocDecode Library.
 *
 * \defgroup group_amd_rocdecode_videodemuxer videoDemuxer: AMD rocDecode Video Demuxer API
 * \brief AMD The rocDecode video demuxer API.
 */
 
  
#include <pybind11/pybind11.h>	// Necessary from pybind11

#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <pybind11/embed.h>
#include <pybind11/eval.h>

namespace py = pybind11;
  
//
// Generic data provider
//
class StreamProvider {
    public:
        virtual ~StreamProvider() {}
        virtual int GetData(uint8_t *buf, int buf_size) = 0;
};

//
// AMD Video Demuxer Interface class
//
class pyVideoDemuxer {
    public:
        AVCodecID GetCodecID() { return av_video_codec_id_; };
       
        pyVideoDemuxer(const char *input_file_path) : pyVideoDemuxer(CreateFmtContextUtil(input_file_path)) {}
        pyVideoDemuxer(StreamProvider *stream_provider) : pyVideoDemuxer(CreateFmtContextUtil(stream_provider)) {av_io_ctx_ = av_fmt_input_ctx_->pb;}
        virtual ~pyVideoDemuxer();
        bool Demux(uint8_t **video, int *video_size, int64_t *pts = nullptr);
        const uint32_t GetWidth() const { return width_;}
        const uint32_t GetHeight() const { return height_;}
        const uint32_t GetChromaHeight() const { return chroma_height_;}
        const uint32_t GetBitDepth() const { return bit_depth_;}
        const uint32_t GetBytePerPixel() const { return byte_per_pixel_;}
        const uint32_t GetBitRate() const { return bit_rate_;}
        const double GetFrameRate() const {return frame_rate_.den != 0 ? frame_rate_.num / frame_rate_.den : 0;};
				
        // for python binding
        virtual bool DemuxFrame(py::array_t<uint64_t>& frame_adrs, py::array_t<int64_t>& frame_size, py::array_t<int64_t>& pts_in) = 0;
        virtual AVCodecID GetCodec_ID() = 0;
     		
	private:
  
        pyVideoDemuxer(AVFormatContext *av_fmt_input_ctx);
        AVFormatContext *CreateFmtContextUtil(StreamProvider *stream_provider);
        AVFormatContext *CreateFmtContextUtil(const char *input_file_path);
        static int ReadPacket(void *data, uint8_t *buf, int buf_size);
        AVFormatContext *av_fmt_input_ctx_ = nullptr;
        AVIOContext *av_io_ctx_ = nullptr;
        AVPacket* packet_ = nullptr;
        AVPacket* packet_filtered_ = nullptr;
        AVBSFContext *av_bsf_ctx_ = nullptr;
        AVCodecID av_video_codec_id_;
        AVPixelFormat chroma_format_;
        AVRational frame_rate_ = {};
        uint8_t *data_with_header_ = nullptr;
        int av_stream_ = 0;
        bool is_h264_ = false; 
        bool is_hevc_ = false;
        bool is_mpeg4_ = false;
        int64_t default_time_scale_ = 1000;
        double time_base_ = 0.0;
        uint32_t frame_count_ = 0;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t chroma_height_ = 0;
        uint32_t bit_depth_ = 0;
        uint32_t byte_per_pixel_ = 0;
        uint32_t bit_rate_ = 0;
};

//
// User defined Demux Process
//
class usrVideoDemuxer : public pyVideoDemuxer {

    public: 

        usrVideoDemuxer(const char *input_file_path) : pyVideoDemuxer(input_file_path){};
        usrVideoDemuxer(StreamProvider *stream_provider) : pyVideoDemuxer(stream_provider){};
        ~usrVideoDemuxer(){};

        bool DemuxFrame(py::array_t<uint64_t>& frame_adrs, py::array_t<int64_t>& frame_size, py::array_t<int64_t>& pts_in) override;
        AVCodecID GetCodec_ID() override;
};

