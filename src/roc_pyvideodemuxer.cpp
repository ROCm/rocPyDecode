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
   
#include "../inc/roc_pyvideodemuxer.h"

using namespace std;

void Init_pyVideoDemuxer(py::module& m)
{
        py::class_<pyVideoDemuxer, std::shared_ptr<pyVideoDemuxer>> (m, "usrVideoDemuxer")
        .def(py::init<const char*>())
        .def("GetCodec_ID",&pyVideoDemuxer::GetCodec_ID,"Get Codec ID")
        .def("DemuxFrame",&pyVideoDemuxer::DemuxFrame);
}

rocDecVideoCodec ConvertAVCodec2RocDecVideoCodec(AVCodecID av_codec)
{
    return AVCodec2RocDecVideoCodec(av_codec);
}

bool pyVideoDemuxer::DemuxFrame(py::array_t<uint64_t>& frame_adrs, py::array_t<int64_t>& frame_size, py::array_t<int64_t>& pts_in) {

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

AVCodecID pyVideoDemuxer::GetCodec_ID() 
{
    return GetCodecID();
}


