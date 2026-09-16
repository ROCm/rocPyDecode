// Copyright © Advanced Micro Devices, Inc., or its affiliates.
 
// SPDX-License-Identifier:  [MIT License]

// test_calls.cpp
#include "roc_pyvideodemuxer.h"
#include "roc_pyvideodecode.h"
#include "roc_pyvideodecodecpu.h"
#include "roc_pybuffer.h"
#include "roc_pydlpack.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>

#ifndef NDEBUG
namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Decoder>
std::shared_ptr<PyPacketData> DecodeFirstFrame(Decoder& decoder, PyVideoDemuxer& demuxer) {
    while (true) {
        auto packet = demuxer.DemuxFrame();
        if (decoder.PyDecodeFrame(*packet) > 0) return packet;
        Require(packet->bitstream_size > 0, "API smoke test did not decode a frame");
    }
}

template <typename Decoder>
void TestDecodedFrame(Decoder& decoder, PyVideoDemuxer& demuxer) {
    auto packet = DecodeFirstFrame(decoder, demuxer);
    Require(decoder.PyGetFrameRgb(*packet, 3).template cast<int64_t>() != -1,
            "API smoke test did not retrieve an RGB frame");
    uintptr_t surface = decoder.PyGetOutputSurfaceInfo();
    Require(surface != 0, "Decoded surface information is missing");
    Dim resized{64, 48};
    if (decoder.PyGetWidth().template cast<int>() == resized.w &&
        decoder.PyGetHeight().template cast<int>() == resized.h) resized.w = 32;
    Require(decoder.PyResizeFrame(*packet, &resized, surface) != 0,
            "API smoke test did not resize a frame");
    Require(decoder.PyGetResizedOutputSurfaceInfo() != 0, "Resized surface information is missing");
    decoder.PyGetDeviceinfo();
    decoder.PyGetNumOfFlushedFrames();
    Require(decoder.PyGetBitDepth() >= 8, "Decoded bit depth is invalid");
    Require(decoder.PyReleaseFrame(*packet).template cast<bool>(), "Frame release failed");
}
}
#endif

void TestAllClassCalls([[maybe_unused]] const char* input_file) {
#ifndef NDEBUG
    Require(input_file && *input_file, "API smoke test requires an input file");
    PyVideoDemuxer demuxer(input_file);
    const auto codec = ConvertAVCodec2RocDecVideoCodec(demuxer.GetCodecId());
    PyRocVideoDecoder decoder(0, OUT_SURFACE_MEM_DEV_COPIED, codec);
    Require(decoder.PyCodecSupported(0, codec, demuxer.PyGetBitDepth()).cast<bool>(),
            "GPU does not support the API smoke-test input");
    TestDecodedFrame(decoder, demuxer);
    std::string output_name;
    decoder.PySetReconfigParams(RECONFIG_FLUSH_MODE_NONE, output_name);

    // Use a fresh stream and device output for CPU API coverage. The SDK's
    // separate host-copy path is not exercised by this API smoke test.
    PyVideoDemuxer cpu_demuxer(input_file);
    PyRocVideoDecoderCpu cpu_decoder(0, OUT_SURFACE_MEM_DEV_COPIED, codec);
    Require(cpu_decoder.PyCodecSupported(0, codec, cpu_demuxer.PyGetBitDepth()).cast<bool>(),
            "CPU decoder does not support the API smoke-test input");
    TestDecodedFrame(cpu_decoder, cpu_demuxer);
    std::cout << "GPU and CPU API smoke tests decoded, resized, and released frames.\n";
#endif
}

void TestAll_roc_pybuffer() {
#ifndef NDEBUG
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
#endif //#ifndef NDEBUG
}

void Test_DLPackPyTensor_ConstructorsAndOperators() {
#ifndef NDEBUG
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
#endif //#ifndef NDEBUG
}

// The actual test
void Test_PyReconfigureFlushCallback([[maybe_unused]] const char* input_file, [[maybe_unused]] const std::string& output_directory) {
#ifndef NDEBUG
    Require(input_file && *input_file, "Flush smoke test requires an input file");
    Require(PyReconfigureFlushCallback(nullptr, 0, nullptr) == 0, "Null flush callback failed");

    // Build expected packed YUV bytes directly from a separate decode, without
    // using the dump callback or SaveFrameToFile as the reference writer.
    std::string expected;
    int expected_frames = 0;
    {
        PyVideoDemuxer demuxer(input_file);
        PyRocVideoDecoder reference(0, OUT_SURFACE_MEM_DEV_COPIED,
            ConvertAVCodec2RocDecVideoCodec(demuxer.GetCodecId()));
        DecodeFirstFrame(reference, demuxer);
        OutputSurfaceInfo* info = nullptr;
        Require(reference.GetOutputSurfaceInfo(&info), "Reference surface information is missing");
        const auto layout = rocpy::Planes(*info);
        std::vector<uint8_t> host(info->output_surface_size_in_bytes);
        int64_t pts = 0;
        while (auto* frame = reference.GetFrame(&pts)) {
            HIP_API_CALL(hipMemcpy(host.data(), frame, host.size(), hipMemcpyDeviceToHost));
            for (size_t i = 0; i < layout.count; ++i) {
                const auto& plane = layout.planes[i];
                const size_t row_bytes = size_t(plane.width) * plane.channels * info->bytes_per_pixel;
                for (uint32_t row = 0; row < plane.height; ++row)
                    expected.append(reinterpret_cast<const char*>(host.data() + plane.offset + row * plane.pitch),
                                    row_bytes);
            }
            Require(reference.ReleaseFrame(pts), "Reference frame release failed");
            ++expected_frames;
        }
    }
    Require(expected_frames > 0 && !expected.empty(), "Reference decode produced no frame data");
    const auto read_output = [](const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        Require(file.is_open(), "Dump callback did not create an output file");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };

    // Cover no-output mode, explicitly disabled dumping, and actual file output.
    for (int scenario = 0; scenario < 3; ++scenario) {
        const auto mode = scenario == 0 ? RECONFIG_FLUSH_MODE_NONE : RECONFIG_FLUSH_MODE_DUMP_TO_FILE;
        const bool should_dump = scenario == 2;
        const auto path = (std::filesystem::path(output_directory) /
                           ("flush-" + std::to_string(scenario) + ".yuv")).string();
        Require(!std::filesystem::exists(path), "Flush test requires a fresh output path");
        PyVideoDemuxer demuxer(input_file);
        PyRocVideoDecoder decoder(0, OUT_SURFACE_MEM_DEV_COPIED,
            ConvertAVCodec2RocDecVideoCodec(demuxer.GetCodecId()));
        DecodeFirstFrame(decoder, demuxer);
        ReconfigDumpFileStruct dump{};
        dump.output_file_name = path;
        dump.b_dump_frames_to_file = scenario != 1;
        Require(PyReconfigureFlushCallback(&decoder, mode, &dump) == expected_frames,
                "Flush callback did not drain the expected frames");
        // Close the SDK writer so buffered output is visible to the file check.
        decoder.ResetSaveFrameToFile();
        if (should_dump)
            Require(read_output(path) == expected, "Dumped frame bytes do not match the reference decode");
        else
            Require(!std::filesystem::exists(path), "Non-dumping flush unexpectedly created a file");

        Require(PyReconfigureFlushCallback(&decoder, mode, &dump) == 0,
                "Flush callback retained frames after draining");
        decoder.ResetSaveFrameToFile();
        if (should_dump)
            Require(read_output(path) == expected, "Empty flush changed the dumped frame data");
        else
            Require(!std::filesystem::exists(path), "Empty flush unexpectedly created a file");
    }
    std::cout << "Flush callback output matches decoded bytes; empty flush leaves output unchanged.\n";
#endif
}

void Test_CalculateRgbImageSize() {
#ifndef NDEBUG
    PyRocVideoDecoderCpu cpu(0, OUT_SURFACE_MEM_DEV_COPIED, rocDecVideoCodec_HEVC);
    PyRocVideoDecoder gpu(0, OUT_SURFACE_MEM_DEV_INTERNAL, rocDecVideoCodec_HEVC);
    const auto check = [](auto& decoder) {
        for (uint32_t depth : {8u, 10u}) {
            OutputSurfaceInfo info{};
            info.bit_depth = depth;
            info.output_width = 1919;
            info.output_height = 1080;
            // Expected allocations include one alignment pixel per row.
            const size_t expected[] = {6220800, 12441600, 6220800, 12441600,
                                       8294400, 16588800, 8294400, 16588800};
            for (int value = 1; value <= 8; ++value) {
                auto format = static_cast<OutputFormatEnum>(value);
                Require(decoder.CalculateRgbImageSize(format, &info) == expected[value - 1],
                        "Incorrect RGB allocation size");
            }
            for (int value : {0, 9, 999}) {
                auto format = static_cast<OutputFormatEnum>(value);
                bool rejected = false;
                try {
                    decoder.CalculateRgbImageSize(format, &info);
                } catch (const std::invalid_argument&) {
                    rejected = true;
                }
                Require(rejected, "Invalid RGB format was accepted");
            }
        }
        OutputSurfaceInfo info{};
        info.output_height = 1;
        for (int value = 1; value <= 8; ++value) {
            auto format = static_cast<OutputFormatEnum>(value);
            const uint32_t bytes_per_pixel = (value >= 5 ? 4u : 3u) * (value % 2 == 0 ? 2u : 1u);
            const uint32_t max_width = (static_cast<uint32_t>(std::numeric_limits<int>::max()) /
                                        bytes_per_pixel) & ~1u;
            for (uint32_t width : {max_width - 1, max_width}) {
                info.output_width = width;
                Require(decoder.CalculateRgbImageSize(format, &info) == size_t(max_width) * bytes_per_pixel,
                        "Largest representable RGB pitch was rejected or changed");
            }
            const uint32_t wrapped_width = (std::numeric_limits<uint32_t>::max() / bytes_per_pixel & ~1u) + 2;
            for (uint32_t width : {max_width + 1, max_width + 2, wrapped_width,
                                   static_cast<uint32_t>(std::numeric_limits<int>::max()),
                                   std::numeric_limits<uint32_t>::max()}) {
                info.output_width = width;
                bool rejected = false;
                try {
                    decoder.CalculateRgbImageSize(format, &info);
                } catch (const std::invalid_argument&) {
                    rejected = true;
                }
                Require(rejected, "Unrepresentable RGB pitch was accepted");
            }
        }
    };
    check(cpu);
    check(gpu);
    std::cout << "RGB allocation sizes, pitch boundaries, and invalid-format rejection verified.\n";
#endif
}
