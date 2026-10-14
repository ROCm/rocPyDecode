// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
 
// SPDX-License-Identifier:  [MIT License]

#include "roc_pyvideodecode.h"
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
// Native-only assertions that cannot be reached through the public Python API.
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
#endif

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

// Python prepares both decoders through its public demux/decode calls.
void Test_PyReconfigureFlushCallback([[maybe_unused]] PyRocVideoDecoder& reference,
                                    [[maybe_unused]] PyRocVideoDecoder& decoder,
                                    [[maybe_unused]] int scenario,
                                    [[maybe_unused]] const std::string& output_directory) {
#ifndef NDEBUG
    Require(scenario >= 0 && scenario < 3, "Unknown flush test scenario");
    Require(PyReconfigureFlushCallback(nullptr, 0, nullptr) == 0, "Null flush callback failed");

    // Build expected packed YUV bytes without the dump callback or file writer.
    std::string expected;
    int expected_frames = 0;
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
    Require(expected_frames > 0 && !expected.empty(), "Reference decode produced no frame data");
    const auto read_output = [](const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        Require(file.is_open(), "Dump callback did not create an output file");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };

    const auto mode = scenario == 0 ? RECONFIG_FLUSH_MODE_NONE : RECONFIG_FLUSH_MODE_DUMP_TO_FILE;
    const bool should_dump = scenario == 2;
    const auto path = (std::filesystem::path(output_directory) /
                       ("flush-" + std::to_string(scenario) + ".yuv")).string();
    Require(!std::filesystem::exists(path), "Flush test requires a fresh output path");
    ReconfigDumpFileStruct dump{};
    dump.output_file_name = path;
    dump.b_dump_frames_to_file = scenario != 1;
    Require(PyReconfigureFlushCallback(&decoder, mode, &dump) == expected_frames,
            "Flush callback did not drain the expected frames");
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
    std::cout << "Flush callback output matches decoded bytes; empty flush leaves output unchanged.\n";
#endif
}

void Test_CalculateRgbImageSize() {
#ifndef NDEBUG
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
    check(gpu);
    std::cout << "RGB allocation sizes, pitch boundaries, and invalid-format rejection verified.\n";
#endif
}
