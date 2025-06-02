
// test_calls.cpp
#include "roc_pyvideodemuxer.h"
#include "roc_pyvideodecode.h"
#include "roc_pyvideodecodecpu.h"
#include "common/roc_pybuffer.h"
#include <iostream>

void TestAllClassCalls(const char* input_file) {
#ifndef NDEBUG    
    // here check input_file, ret if invalid str or null
    if(!input_file){
        std::cout << "ERROR: Input File Name is a nullptr, BAD argument sent." << std::endl;    
        return;
    }
    std::cout << "Input File Full Path Name: " << input_file << std::endl;

    // Initialize and test demuxer
    PyVideoDemuxer demuxer(input_file);
    std::cout << "Testing PyVideoDemuxer...\n";
    int codec = demuxer.GetCodecId();
    uint32_t depth = demuxer.PyGetBitDepth();
    std::shared_ptr<PyPacketData> pkt1 = demuxer.SeekFrame(0, 1, 0);
    std::shared_ptr<PyPacketData> pkt2 = demuxer.DemuxFrame();
    (void)codec; (void)depth; (void)pkt1; (void)pkt2;

    int codec_id = demuxer.GetCodecId();
    uint32_t bit_depth = demuxer.PyGetBitDepth();
    rocDecVideoCodec dec_codec = ConvertAVCodec2RocDecVideoCodec(codec_id);

    int device_id = 0;
    int mem_type = 1;
    bool force_zero_latency = false;

    // test GPU decoder
    PyRocVideoDecoder viddec(device_id, mem_type, dec_codec, force_zero_latency);
    std::cout << "Testing PyRocVideoDecoder...\n";
    std::shared_ptr<PyPacketData> pkt = demuxer.DemuxFrame();
    if (!viddec.PyCodecSupported(0, ConvertAVCodec2RocDecVideoCodec(demuxer.GetCodecId()), demuxer.PyGetBitDepth()).cast<bool>()) return;
    viddec.PyGetFrameRgb(*pkt, 3);
    Dim resize_dim{1920, 1080};
    uintptr_t surf_info = static_cast<uintptr_t>(0); 
    viddec.PyResizeFrame(*pkt, &resize_dim, surf_info);
    viddec.PyGetResizedOutputSurfaceInfo();    
    viddec.PyGetDeviceinfo();    
    viddec.PyGetNumOfFlushedFrames();
    std::string empty_name = "";
    viddec.PySetReconfigParams(0, empty_name);
    viddec.PyGetBitDepth();
    viddec.PyReleaseFrame(*pkt);

    // test CPU decoder
    PyRocVideoDecoderCpu cpu_dec(device_id, mem_type, dec_codec, force_zero_latency);    
    std::cout << "Testing PyRocVideoDecoderCpu...\n";
    pkt = demuxer.DemuxFrame();
    if (!cpu_dec.PyCodecSupported(0, ConvertAVCodec2RocDecVideoCodec(demuxer.GetCodecId()), demuxer.PyGetBitDepth()).cast<bool>()) return;
    cpu_dec.PyGetFrameRgb(*pkt, 3);    
    cpu_dec.PyResizeFrame(*pkt, &resize_dim, surf_info);
    cpu_dec.PyGetResizedOutputSurfaceInfo();    
    cpu_dec.PyGetDeviceinfo();    
    cpu_dec.PyGetNumOfFlushedFrames();
    cpu_dec.PyGetBitDepth();
    cpu_dec.PyReleaseFrame(*pkt);

    std::cout << "All classes member methods calls completed successfully." << std::endl;
#endif //#ifndef NDEBUG    
}