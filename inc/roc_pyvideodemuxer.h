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
 
#include "video_demuxer.h"
#include "roc_pydecode.h"
 
// include here: /opt/rocm/include/rocdecode                        [rocdecode.h rocparser.h roc_video_dec.h video_demuxer.h << CLASS demux inside]
// CPP here:     /opt/rocm/share/rocdecode/utils/rocvideodecode/    [roc_video_dec.cpp roc_video_dec.h << CLASS in cpp]
 
//
// AMD Video Demuxer Python Interface class
//
class pyVideoDemuxer : public VideoDemuxer {

    protected:
        std::shared_ptr <PacketData> currentPacket;
        void initPacket();

    public:
        pyVideoDemuxer(const char *input_file_path) : VideoDemuxer(input_file_path) { initPacket(); }
        pyVideoDemuxer(VideoDemuxer::StreamProvider *stream_provider) : VideoDemuxer(stream_provider) { initPacket(); }
        				
        // for python binding
        std::shared_ptr<PacketData> DemuxFrame();
        AVCodecID GetCodec_ID();
     	
};

 

