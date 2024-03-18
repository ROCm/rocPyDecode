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
 
#include <iostream>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #if USE_AVCODEC_GREATER_THAN_58_134
        #include <libavcodec/bsf.h>
    #endif
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
 
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <pybind11/embed.h>
#include <pybind11/eval.h>

#include "../inc/roc_pyvideodemuxer.h"

namespace py = pybind11;
  

bool usrVideoDemuxer::DemuxFrame(py::array_t<uint64_t>& frame_adrs, py::array_t<int64_t>& frame_size, py::array_t<int64_t>& pts_in) {

    uint8_t *video=nullptr;
    int video_size=0;
    int64_t pts=0;

    bool ret = Demux(&video, &video_size, &pts); 
  
    int64_t vd_size = video_size;

    frame_adrs.resize({sizeof(uint64_t)}, false);
    memcpy(frame_adrs.mutable_data(), (uint64_t*)&video, sizeof(uint64_t)); // copy the adrs, not the content: Essam

    frame_size.resize({sizeof( int64_t)}, false);
    memcpy(frame_size.mutable_data(), &vd_size, sizeof(int64_t));  

    pts_in.resize({sizeof( int64_t)}, false);
    memcpy(pts_in.mutable_data(), &pts, sizeof(int64_t));

    return ret;
}

AVCodecID usrVideoDemuxer::GetCodec_ID() 
{
    return GetCodecID();
}

pyVideoDemuxer::~pyVideoDemuxer() {
    
    if (!av_fmt_input_ctx_) {
        return;
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
    if (packet_filtered_) {
        av_packet_free(&packet_filtered_);
    }
    if (av_bsf_ctx_) {
        av_bsf_free(&av_bsf_ctx_);
    }
    avformat_close_input(&av_fmt_input_ctx_);
    if (av_io_ctx_) {
        av_freep(&av_io_ctx_->buffer);
        av_freep(&av_io_ctx_);
    }
    if (data_with_header_) {
        av_free(data_with_header_);
    }
}

bool pyVideoDemuxer::Demux(uint8_t **video, int *video_size, int64_t *pts) {
 
    if (!av_fmt_input_ctx_) {
        return false;
    }
    *video_size = 0;
    if (packet_->data) {
        av_packet_unref(packet_);
    }
    int ret = 0;
    while ((ret = av_read_frame(av_fmt_input_ctx_, packet_)) >= 0 && packet_->stream_index != av_stream_) {
        av_packet_unref(packet_);
    }
    if (ret < 0) {
        return false;
    }
    if (is_h264_ || is_hevc_) {
        if (packet_filtered_->data) {
            av_packet_unref(packet_filtered_);
        }
        if (av_bsf_send_packet(av_bsf_ctx_, packet_) != 0) {
            std::cerr << "ERROR: av_bsf_send_packet failed!" << std::endl;
            return false;
        }
        if (av_bsf_receive_packet(av_bsf_ctx_, packet_filtered_) != 0) {
            std::cerr << "ERROR: av_bsf_receive_packet failed!" << std::endl;
            return false;
        }
        *video = packet_filtered_->data;
        *video_size = packet_filtered_->size;
        if (pts)
            *pts = (int64_t) (packet_filtered_->pts * default_time_scale_ * time_base_);
        } else {
           if (is_mpeg4_ && (frame_count_ == 0)) {
               int ext_data_size = av_fmt_input_ctx_->streams[av_stream_]->codecpar->extradata_size;
               if (ext_data_size > 0) {
                    data_with_header_ = (uint8_t *)av_malloc(ext_data_size + packet_->size - 3 * sizeof(uint8_t));
                    if (!data_with_header_) {
                        std::cerr << "ERROR: av_malloc failed!" << std::endl;
                        return false;
                    }
                    memcpy(data_with_header_, av_fmt_input_ctx_->streams[av_stream_]->codecpar->extradata, ext_data_size);
                    memcpy(data_with_header_ + ext_data_size, packet_->data + 3, packet_->size - 3 * sizeof(uint8_t));
                    *video = data_with_header_;
                    *video_size = ext_data_size + packet_->size - 3 * sizeof(uint8_t);
                }
            } else {
                *video = packet_->data;
                *video_size = packet_->size;
            }
            if (pts)
                *pts = (int64_t)(packet_->pts * default_time_scale_ * time_base_);
    }
    frame_count_++;
    return true;
}
 
pyVideoDemuxer::pyVideoDemuxer(AVFormatContext *av_fmt_input_ctx) : av_fmt_input_ctx_(av_fmt_input_ctx) {

    // init, 1st time 
    std::cout << "Info: pyVideoDemuxer Constructor ..\n"; 

    av_log_set_level(AV_LOG_QUIET);
    if (!av_fmt_input_ctx_) {
        std::cerr << "ERROR: av_fmt_input_ctx_ is not vaild!" << std::endl;
        return;
    }
    packet_ = av_packet_alloc();
    packet_filtered_ = av_packet_alloc();
    if (!packet_ || !packet_filtered_) {
        std::cerr << "ERROR: av_packet_alloc failed!" << std::endl;
        return;
    }
    if (avformat_find_stream_info(av_fmt_input_ctx_, nullptr) < 0) {
        std::cerr << "ERROR: avformat_find_stream_info failed!" << std::endl;
        return;
    }
    av_stream_ = av_find_best_stream(av_fmt_input_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (av_stream_ < 0) {
        std::cerr << "ERROR: av_find_best_stream failed!" << std::endl;
        av_packet_free(&packet_);
        av_packet_free(&packet_filtered_);
        return;
    }
    av_video_codec_id_ = av_fmt_input_ctx_->streams[av_stream_]->codecpar->codec_id;
    width_ = av_fmt_input_ctx_->streams[av_stream_]->codecpar->width;
    height_ = av_fmt_input_ctx_->streams[av_stream_]->codecpar->height;
    chroma_format_ = (AVPixelFormat)av_fmt_input_ctx_->streams[av_stream_]->codecpar->format;
    bit_rate_ = av_fmt_input_ctx_->streams[av_stream_]->codecpar->bit_rate;
    frame_rate_ = av_fmt_input_ctx_->streams[av_stream_]->r_frame_rate;

    switch (chroma_format_) {
        case AV_PIX_FMT_YUV420P10LE:
        case AV_PIX_FMT_GRAY10LE:
            bit_depth_ = 10;
            chroma_height_ = (height_ + 1) >> 1;
            byte_per_pixel_ = 2;
            break;
        case AV_PIX_FMT_YUV420P12LE:
            bit_depth_ = 12;
            chroma_height_ = (height_ + 1) >> 1;
            byte_per_pixel_ = 2;
            break;
        case AV_PIX_FMT_YUV444P10LE:
            bit_depth_ = 10;
            chroma_height_ = height_ << 1;
            byte_per_pixel_ = 2;
            break;
        case AV_PIX_FMT_YUV444P12LE:
            bit_depth_ = 12;
            chroma_height_ = height_ << 1;
            byte_per_pixel_ = 2;
            break;
        case AV_PIX_FMT_YUV444P:
            bit_depth_ = 8;
            chroma_height_ = height_ << 1;
            byte_per_pixel_ = 1;
            break;
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
        case AV_PIX_FMT_YUVJ422P:
        case AV_PIX_FMT_YUVJ444P:
        case AV_PIX_FMT_GRAY8:
            bit_depth_ = 8;
            chroma_height_ = (height_ + 1) >> 1;
            byte_per_pixel_ = 1;
            break;
        default:
            chroma_format_ = AV_PIX_FMT_YUV420P;
            bit_depth_ = 8;
            chroma_height_ = (height_ + 1) >> 1;
            byte_per_pixel_ = 1;
        }

    AVRational time_base = av_fmt_input_ctx_->streams[av_stream_]->time_base;
    time_base_ = av_q2d(time_base);

    is_h264_ = av_video_codec_id_ == AV_CODEC_ID_H264 && (!strcmp(av_fmt_input_ctx_->iformat->long_name, "QuickTime / MOV") 
                || !strcmp(av_fmt_input_ctx_->iformat->long_name, "FLV (Flash Video)") 
                || !strcmp(av_fmt_input_ctx_->iformat->long_name, "Matroska / WebM"));
    is_hevc_ = av_video_codec_id_ == AV_CODEC_ID_HEVC && (!strcmp(av_fmt_input_ctx_->iformat->long_name, "QuickTime / MOV")
                || !strcmp(av_fmt_input_ctx_->iformat->long_name, "FLV (Flash Video)")
                || !strcmp(av_fmt_input_ctx_->iformat->long_name, "Matroska / WebM"));
    is_mpeg4_ = av_video_codec_id_ == AV_CODEC_ID_MPEG4 && (!strcmp(av_fmt_input_ctx_->iformat->long_name, "QuickTime / MOV")
                || !strcmp(av_fmt_input_ctx_->iformat->long_name, "FLV (Flash Video)")
                || !strcmp(av_fmt_input_ctx_->iformat->long_name, "Matroska / WebM"));

    if (is_h264_) {
        const AVBitStreamFilter *bsf = av_bsf_get_by_name("h264_mp4toannexb");
        if (!bsf) {
            std::cerr << "ERROR: av_bsf_get_by_name() failed" << std::endl;
            av_packet_free(&packet_);
            av_packet_free(&packet_filtered_);
            return;
        }
        if (av_bsf_alloc(bsf, &av_bsf_ctx_) != 0) {
            std::cerr << "ERROR: av_bsf_alloc failed!" << std::endl;
                return;
        }
        avcodec_parameters_copy(av_bsf_ctx_->par_in, av_fmt_input_ctx_->streams[av_stream_]->codecpar);
        if (av_bsf_init(av_bsf_ctx_) < 0) {
            std::cerr << "ERROR: av_bsf_init failed!" << std::endl;
            return;
        }
    }
    if (is_hevc_) {
        const AVBitStreamFilter *bsf = av_bsf_get_by_name("hevc_mp4toannexb");
        if (!bsf) {
            std::cerr << "ERROR: av_bsf_get_by_name() failed" << std::endl;
            av_packet_free(&packet_);
            av_packet_free(&packet_filtered_);
            return;
        }
        if (av_bsf_alloc(bsf, &av_bsf_ctx_) != 0 ) {
            std::cerr << "ERROR: av_bsf_alloc failed!" << std::endl;
            return;
        }
        avcodec_parameters_copy(av_bsf_ctx_->par_in, av_fmt_input_ctx_->streams[av_stream_]->codecpar);
        if (av_bsf_init(av_bsf_ctx_) < 0) {
            std::cerr << "ERROR: av_bsf_init failed!" << std::endl;
            return;
        }
    }
}

AVFormatContext *pyVideoDemuxer::CreateFmtContextUtil(StreamProvider *stream_provider) {
    AVFormatContext *ctx = nullptr;
    if (!(ctx = avformat_alloc_context())) {
        std::cerr << "ERROR: avformat_alloc_context failed" << std::endl;
        return nullptr;
    }
    uint8_t *avioc_buffer = nullptr;
    int avioc_buffer_size = 100 * 1024 * 1024;
    avioc_buffer = (uint8_t *)av_malloc(avioc_buffer_size);
    if (!avioc_buffer) {
        std::cerr << "ERROR: av_malloc failed!" << std::endl;
        return nullptr;
    }
    av_io_ctx_ = avio_alloc_context(avioc_buffer, avioc_buffer_size,
        0, stream_provider, &ReadPacket, nullptr, nullptr);
    if (!av_io_ctx_) {
        std::cerr << "ERROR: avio_alloc_context failed!" << std::endl;
        return nullptr;
    }
    ctx->pb = av_io_ctx_;

    if (avformat_open_input(&ctx, nullptr, nullptr, nullptr) != 0) {
        std::cerr << "ERROR: avformat_open_input failed!" << std::endl;
        return nullptr;
    }
    return ctx;
}

AVFormatContext *pyVideoDemuxer::CreateFmtContextUtil(const char *input_file_path) {
    avformat_network_init();
    AVFormatContext *ctx = nullptr;
    if (avformat_open_input(&ctx, input_file_path, nullptr, nullptr) != 0 ) {
        std::cerr << "ERROR: avformat_open_input failed!" << std::endl;
        return nullptr;
    }
    return ctx;
}

int pyVideoDemuxer::ReadPacket(void *data, uint8_t *buf, int buf_size) {
    return ((StreamProvider *)data)->GetData(buf, buf_size);
}


