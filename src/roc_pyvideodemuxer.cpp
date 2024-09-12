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
   
#include "roc_pyvideodemuxer.h"
#include <exception>

using namespace std;

void PyVideoDemuxerInitializer(py::module& m) {
        py::class_<PyVideoDemuxer, std::shared_ptr<PyVideoDemuxer>> (m, "PyVideoDemuxer")
        .def(py::init<const char*>())
        .def(py::init<PyFileStreamProvider *>())
        .def("GetCodecId",&PyVideoDemuxer::GetCodecId,"Get Codec ID")
        .def("DemuxFrame",&PyVideoDemuxer::DemuxFrame)
        .def("SeekFrame",&PyVideoDemuxer::SeekFrame)
        .def("GetBitDepth",&PyVideoDemuxer::PyGetBitDepth);
}

void PyVideoStreamProviderInitializer(py::module& m) {
        py::class_<PyFileStreamProvider, std::shared_ptr<PyFileStreamProvider>> (m, "PyFileStreamProvider")
        .def(py::init<const char*>())
        .def("GetData",&PyFileStreamProvider::GetData,"Get IO Data")
        .def("GetBufferSize",&PyFileStreamProvider::GetBufferSize, "Get IO Buffer size");
}

rocDecVideoCodec ConvertAVCodec2RocDecVideoCodec(int av_codec) {
    return AVCodec2RocDecVideoCodec((AVCodecID)av_codec);
}

void PyVideoDemuxer::InitPacket() {
    currentPacket.reset(new PyPacketData());    
    currentPacket.get()->frame_adrs = 0;
    currentPacket.get()->frame_adrs_rgb = 0;
    currentPacket.get()->pkt_flags = 0;
    currentPacket.get()->frame_size = 0;
    currentPacket.get()->frame_pts = 0;
    currentPacket.get()->end_of_stream = false;
    currentPacket.get()->extBuf.reset(new BufferInterface());
}

shared_ptr<PyPacketData> PyVideoDemuxer::DemuxFrame() {
    uint8_t *p_video = nullptr;
    int video_size = 0;
    int64_t pts = 0;
        
    bool ret = Demux(&p_video, &video_size, &pts);
    currentPacket.get()->frame_adrs = reinterpret_cast<std::uintptr_t>(p_video);
    currentPacket.get()->frame_size = video_size;
    currentPacket.get()->frame_pts = pts;
    currentPacket.get()->end_of_stream = !ret;
    return currentPacket;
}

shared_ptr<PyPacketData> PyVideoDemuxer::SeekFrame(int frame_number, int seek_mode, int seek_criteria) {
    uint8_t *p_video = nullptr;
    int video_size = 0;

    VideoSeekContext video_seek;
    video_seek.seek_frame_ = frame_number;
    video_seek.seek_mode_ = static_cast<SeekMode>(seek_mode);
    video_seek.seek_crit_ = static_cast<SeekCriteria>(seek_criteria);

    bool ret = false;
    try {
        ret = Seek(video_seek, &p_video, &video_size);
    } catch (const std::exception &ex) {
      std::cerr << "Seek call failed: " << ex.what() << std::endl;
      exit(1);
    }

    currentPacket.get()->frame_adrs = reinterpret_cast<std::uintptr_t>(p_video);
    currentPacket.get()->frame_size = video_size;
    currentPacket.get()->frame_pts = video_seek.out_frame_pts_;
    currentPacket.get()->end_of_stream = !ret;
    return currentPacket;
}

int PyVideoDemuxer::GetCodecId() {
    return GetCodecID();
}

uint32_t PyVideoDemuxer::PyGetBitDepth() {
    return GetBitDepth();
}
