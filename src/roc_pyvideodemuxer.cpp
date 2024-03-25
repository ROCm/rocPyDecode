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

using namespace std;

void pyVideoDemuxerInitializer(py::module& m) {
        py::class_<pyVideoDemuxer, std::shared_ptr<pyVideoDemuxer>> (m, "usrVideoDemuxer")
        .def(py::init<const char*>())
        .def("GetCodec_ID",&pyVideoDemuxer::GetCodec_ID,"Get Codec ID")
        .def("DemuxFrame",&pyVideoDemuxer::DemuxFrame);
}

rocDecVideoCodec ConvertAVCodec2RocDecVideoCodec(AVCodecID av_codec) {
    return AVCodec2RocDecVideoCodec(av_codec);
}

void pyVideoDemuxer::initPacket() {
    currentPacket.reset(new PacketData());    
    currentPacket.get()->frame_adrs = (uintptr_t)nullptr;
    currentPacket.get()->frame_size = 0;
    currentPacket.get()->frame_pts = 0;
    currentPacket.get()->end_of_stream = false;
}

shared_ptr<PacketData> pyVideoDemuxer::DemuxFrame() {

    uint8_t *pVideo=nullptr;
    int video_size=0;
    int64_t pts=0;
        
    if(Demux(&pVideo, &video_size, &pts)) {
        if(video_size>0) {
            currentPacket.get()->frame_adrs = (uintptr_t)pVideo;
            currentPacket.get()->frame_size = video_size;
            currentPacket.get()->frame_pts = pts;
        } else {
            currentPacket.get()->end_of_stream = true;        
        }
    } else {
        currentPacket.get()->end_of_stream = true;
    }

    return currentPacket;
}

AVCodecID pyVideoDemuxer::GetCodec_ID() {
    return GetCodecID();
}


