
// test_calls.cpp
#include "roc_pyvideodemuxer.h"
#include "roc_pyvideodecode.h"
#include "roc_pyvideodecodecpu.h"
#include "common/roc_pybuffer.h"
#include "common/roc_pydlpack.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>

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

void TestAll_roc_pybuffer() {
    std::cout << "Running TestAll_roc_pybuffer()..." << std::endl;

    // Step 1: Create test tensor via DLPackPyTensor
    std::vector<size_t> shape = {2, 3};
    std::vector<size_t> strides = {3, 1};
    std::vector<uint8_t> buffer(6, 42);

    py::buffer_info info(buffer.data(), sizeof(uint8_t), py::format_descriptor<uint8_t>::format(), 2, shape, strides);
    DLDevice dev;
    dev.device_type = kDLCPU;
    dev.device_id = 0;
    DLPackPyTensor tensor(info, dev);

    // Step 2: Construct BufferInterface
    std::shared_ptr<BufferInterface> buf = std::make_shared<BufferInterface>(std::move(tensor));

    // Step 3: Call shape()
    auto sh = buf->shape();
    std::cout << "Shape: (" << sh[0].cast<int>() << ", " << sh[1].cast<int>() << ")\n";

    // Step 4: Call strides()
    auto st = buf->strides();
    std::cout << "Strides: (" << st[0].cast<int>() << ", " << st[1].cast<int>() << ")\n";

    // Step 5: dtype()
    std::string dtype = buf->dtype();
    std::cout << "Dtype: " << dtype << std::endl;

    // Step 6: data()
    void* data_ptr = buf->data();
    std::cout << "Data ptr: " << data_ptr << std::endl;

    // Step 7: dlTensor()
    const DLTensor& t = buf->dlTensor();
    std::cout << "DLTensor ndim: " << t.ndim << ", device_id: " << t.device.device_id << std::endl;

    // Step 8: dlpack() and dlpackDevice()
    auto cap = buf->dlpack(py::int_(1));
    auto devinfo = buf->dlpackDevice();
    std::cout << "Device tuple: (" << devinfo[0].cast<int>() << ", " << devinfo[1].cast<int>() << ")\n";

    // Step 9: Test LoadDLPack()
    std::shared_ptr<BufferInterface> buf2 = std::make_shared<BufferInterface>();
    std::string typstr = "|u1";
    int ret = buf2->LoadDLPack(shape, strides, 8, typstr, buffer.data(), 0);
    std::cout << "LoadDLPack returned: " << ret << std::endl;

    std::cout << "TestAll_roc_pybuffer() completed successfully.\n";
}

void Test_DLPackPyTensor_ConstructorsAndOperators() {
    std::cout << "Testing DLPackPyTensor constructors and operators...\n";

    // Step 1: Manually create DLTensor
    std::vector<int64_t> shape = {2, 3};
    std::vector<int64_t> strides = {3, 1};
    std::vector<uint8_t> data(6, 99);

    DLDevice device;
    device.device_type = kDLCPU;
    device.device_id = 0;

    DLTensor tensor;
    tensor.data = data.data();
    tensor.device = device;
    tensor.ndim = 2;
    tensor.dtype = DLDataType{uint8_t(1), 8, 1};
    tensor.shape = shape.data();
    tensor.strides = strides.data();
    tensor.byte_offset = 0;

    // Step 2: Test constructor: DLPackPyTensor(const DLTensor&)
    DLPackPyTensor tensor_from_dl(tensor);
    std::cout << "Constructed DLPackPyTensor from DLTensor.\n";

    // Step 3: Access underlying tensor using operator->
    const DLTensor* ptr = tensor_from_dl.operator->();
    std::cout << "Accessed ndim via operator->: " << ptr->ndim << std::endl;

    // Step 4: Access underlying tensor using operator*
    const DLTensor& ref = *tensor_from_dl;
    std::cout << "Accessed shape[0] via operator*: " << ref.shape[0] << std::endl;

    // Step 5: Test move constructor DLPackPyTensor(DLManagedTensor&&)
    DLManagedTensor m_tensor;
    m_tensor.dl_tensor = tensor; // Copy the tensor struct
    m_tensor.manager_ctx = nullptr;
    m_tensor.deleter = nullptr;

    DLPackPyTensor moved_tensor(std::move(m_tensor));
    std::cout << "Constructed DLPackPyTensor by moving DLManagedTensor.\n";

    // Confirm internal access still works
    std::cout << "Moved tensor shape[1]: " << moved_tensor->shape[1] << std::endl;

    std::cout << "All DLPackPyTensor constructor/operator tests passed.\n";
}

// The actual test
void Test_PyReconfigureFlushCallback() {
    int device_id = 0;
    OutputSurfaceMemoryType mem_type = static_cast<OutputSurfaceMemoryType>(0);
    rocDecVideoCodec codec = rocDecVideoCodec_HEVC;
    bool force_zero_latency = false;

    RocVideoDecoder decoder(
        device_id,
        mem_type,
        codec,
        force_zero_latency,
        nullptr,      // crop_rect
        false,        // extract_user_SEI_Message
        0,            // disp_delay
        1920, 1080,   // max_width, max_height
        1000          // clk_rate
    );

    ReconfigDumpFileStruct dump_struct;
    dump_struct.b_dump_frames_to_file = true;
    dump_struct.output_file_name = "dummy_output.yuv";

    // Call with RECONFIG_FLUSH_MODE_DUMP_TO_FILE
    int flushed = PyReconfigureFlushCallback(&decoder, RECONFIG_FLUSH_MODE_DUMP_TO_FILE, &dump_struct);
    std::cout << "Flushed frames: " << flushed << std::endl;

    // test with nullptr to hit the early-return path
    int flushed_null = PyReconfigureFlushCallback(nullptr, 0, nullptr);
    std::cout << "Flushed frames (null case): " << flushed_null << std::endl;

    // test with RECONFIG_FLUSH_MODE_NONE (won’t call SaveFrameToFile)
    flushed = PyReconfigureFlushCallback(&decoder, RECONFIG_FLUSH_MODE_NONE, &dump_struct);
    std::cout << "Flushed frames (no file save): " << flushed << std::endl;
}