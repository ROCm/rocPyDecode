/*
Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.

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

#include "roc_pyjpeg_decoder.h"
#include "roc_pyjpeg_utils.h"
#include "roc_pyjpeg_codestream.h"
#include "roc_pyjpeg_images.h"
#include <array>

using namespace std;
using namespace py::literals;

void Decoder::ExportToPython(py::module& m) {
    // Decoder Class
    py::class_<Decoder>(m, "Decoder", "Decoder for image decoding operations. "
        "It provides methods to decode images from various sources such as files or data streams. ")
        .def(py::init(
            [](int device_id, int backend, RocJpegOutputFormat output_format = ROCJPEG_OUTPUT_RGB) {
                return new Decoder(device_id, backend, output_format);
            }),
            R"pbdoc(
            Initialize decoder.

            Args:
                device_id: Device id to execute decoding on.

                backend: CPU or GPU backend.

            )pbdoc",
             py::arg("device_id") = 0,  py::arg("backend") = 0, py::arg("output_format") = ROCJPEG_OUTPUT_RGB)
        .def("SetOutputFormat",&Decoder::SetOutputFormat)
        .def("read", py::overload_cast<DecodeSource*>(&Decoder::decode), R"pbdoc(
            Executes decoding from a filename.

            Args:
                path: File path to decode.

            Returns:
                rocPyJpeg.Image or None if the image cannot be decoded because of any reason.
        )pbdoc",
            "path"_a)
        .def("read", py::overload_cast<std::vector<DecodeSource*>&>(&Decoder::decode),
            R"pbdoc(
            Executes decoding from a batch of file paths.

            Args:
                path: List of file paths to decode.

            Returns:
                List of decoded rocPyJpeg.Image's. There is None in returned list on positions which could not be decoded.

            )pbdoc",
            "paths"_a)
        .def("decode", py::overload_cast<DecodeSource*>(&Decoder::decode), R"pbdoc(
            Executes decoding of data from a DecodeSource handle (code stream handle).

            Args:
                src: decode source object.

            Returns:
                rocPyJpeg.Image or None if the image cannot be decoded because of any reason.

            )pbdoc",
            "src"_a)
        .def("decode", py::overload_cast<std::vector<DecodeSource*>&>(&Decoder::decode),
            R"pbdoc(
            Executes decoding from a batch of DecodeSource handles (code stream handle).

            Args:
                srcs: List of DecodeSource objects

            Returns:
                List of decoded rocPyJpeg.Image's. There is None in returned list on positions which could not be decoded.
            )pbdoc",
            "srcs"_a);
}

Decoder::Decoder(int device_id, int backend, RocJpegOutputFormat output_format) {
    SetOutputFormat(output_format);     // save user choice if sent for output format
    m_device_id = device_id;            // save user device id
    m_backend = RocJpegBackend(backend);// save user backend choice
    // create decode obj
    PY_CHECK_ROCJPEG(rocJpegCreate(m_backend, m_device_id, &rocjpeg_handle));    
    SetFormat(output_format); // init
}

// receiving single code_stream
std::pair<float, PyJpegImages> Decoder::decode(DecodeSource* data) {
    float elapsed_ms = 0.0;
    // keep code stream ptr in class (alive)
    assert(data);
    // img 'instance' to return
    PyJpegImages img;
    // get current data/file associated code_stream instance
    const CodeStream* c_stream = data->CodeStreamInstance();    
    // runtime sanity check
    if(!c_stream->stream_handle) {
        return {elapsed_ms, img}; // last item is this instance
    }
    // DECODE one single JPEG image
    if( GetImageInfo(c_stream->stream_handle, img) == EXIT_SUCCESS) {
        auto start = std::chrono::high_resolution_clock::now();
        RocJpegStatus status = rocJpegDecode(rocjpeg_handle, c_stream->stream_handle, &img.decode_params, &img.output_image);
        auto end = std::chrono::high_resolution_clock::now();
        if (status != ROCJPEG_STATUS_SUCCESS) {
            std::cerr << "ERROR: Failed to decode image. Status code: " << status << std::endl;
            return {elapsed_ms, img};
        }
        // to export to python (use dlpack(GPU MEM) {and numpy host array}) -- GPU Tensor
        img.ToDlpackTensor(user_output_format, m_device_id);
        elapsed_ms = std::chrono::duration<float, std::milli>(end - start).count();
    }
    return {elapsed_ms, img}; // return one image
}

// receiving multiple code_streams
std::pair<float, std::vector<PyJpegImages>> Decoder::decode(std::vector<DecodeSource*>& decode_source_arg) {

    float elapsed_ms = 0.0;
    RocJpegStatus status = ROCJPEG_STATUS_SUCCESS;
    const auto batch_size = decode_source_arg.size();
    size_t count_of_valid_instances = 0;
    std::vector<RocJpegStreamHandle> stream_handles;
    std::vector<RocJpegDecodeParams> decode_params_list;
    std::vector<RocJpegImage> destinations;

    // we return a list of images
    std::vector<PyJpegImages> images_;

    if(batch_size == 0U)
        return {elapsed_ms, images_};

    // loop the whole list length - Process as one BATCH
    for (auto* data : decode_source_arg) {       
        if (!data)
            continue;
        // get current data/file associated code_stream instance
        const CodeStream* c_stream = data->CodeStreamInstance();    
        // runtime sanity check
        if(!c_stream->stream_handle)
            continue;
        // one img 'instance'
        PyJpegImages img;
        // add to the list for 'rocJpegDecodeBatched()'
        if( GetImageInfo(c_stream->stream_handle, img) == EXIT_FAILURE) {
            continue;
        }
        stream_handles.push_back(c_stream->stream_handle);
        decode_params_list.push_back(img.decode_params);
        destinations.push_back(img.output_image);
        count_of_valid_instances++;
        images_.push_back(img);
    }

    // at least one image
    if(count_of_valid_instances > 0) {
        // DECODE ALL files/images in the batch one-time
        auto start = std::chrono::high_resolution_clock::now();
        status = rocJpegDecodeBatched(  rocjpeg_handle,
                                        stream_handles.data(),
                                        static_cast<int>(count_of_valid_instances), // less or equal to the batch_size
                                        decode_params_list.data(),
                                        destinations.data()
                                        );
        auto end = std::chrono::high_resolution_clock::now();
        elapsed_ms += std::chrono::duration<float, std::milli>(end - start).count();
        if (status != ROCJPEG_STATUS_SUCCESS) {
            std::cerr << "ERROR: Failed to decode the image batch. Status code: " << status << std::endl;
            return {elapsed_ms, images_}; // return the image list
        }
        // here 'images_' vector carries the count of 'valid' images
        // to export to python (use dlpack(GPU MEM) {and numpy host array})
        if (status == ROCJPEG_STATUS_SUCCESS) {
            for(size_t i = 0; i < count_of_valid_instances; ++i) {
                images_[i].ToDlpackTensor(user_output_format, m_device_id); // GPU Tensor
            }
        }
    }
    return {elapsed_ms, images_}; // return the image list
}

// set the user desired output_format
void Decoder::SetOutputFormat(RocJpegOutputFormat output_format) {
    if(output_format != ROCJPEG_OUTPUT_RGB && output_format != ROCJPEG_OUTPUT_RGB_PLANAR) {
        std::cerr << "ERROR: Unsupported output format, defaulting to ROCJPEG_OUTPUT_RGB." << std::endl;
        user_output_format = ROCJPEG_OUTPUT_RGB; // default
        return;
    }
    user_output_format = output_format;
}

Decoder::~Decoder() {
    // delete/destroy MAIN decode obj
    if(rocjpeg_handle != nullptr) {
        PY_CHECK_ROCJPEG(rocJpegDestroy(rocjpeg_handle));
        rocjpeg_handle = nullptr;
    }
}

// Get Image Info, Pitch, Sizes, and alloc GPU MEM
int Decoder::GetImageInfo(RocJpegStreamHandle stream_handle, PyJpegImages& img) {
    uint8_t num_components = 0;
    std::vector<uint32_t> widths(ROCJPEG_MAX_COMPONENT, 0U);
    std::vector<uint32_t> heights(ROCJPEG_MAX_COMPONENT, 0U);
    std::vector<uint32_t> channel_sizes(ROCJPEG_MAX_COMPONENT, 0U);
    // default, reset
    img.decode_params.output_format = user_output_format;
    // Get the image info
    RocJpegStatus rocjpeg_status = rocJpegGetImageInfo(rocjpeg_handle, stream_handle, &num_components, &img.subsampling, widths.data(), heights.data());
    if (rocjpeg_status != ROCJPEG_STATUS_SUCCESS) {
        std::cerr << "ERROR: Failed to  get image info with " << rocJpegGetErrorName(rocjpeg_status) << std::endl;
        return EXIT_FAILURE;
    }
    // Check limits of w/h & subsampling
    if (widths[0] < 64 || heights[0] < 64) {
        std::cerr << "The image resolution is not supported by VCN Hardware" << std::endl;
        return EXIT_FAILURE;
    }
    if (img.subsampling == ROCJPEG_CSS_411 || img.subsampling == ROCJPEG_CSS_UNKNOWN) {
        std::cerr << "The image resolution is not supported by VCN Hardware" << std::endl;
        return EXIT_FAILURE;
    }    
    // save the output w/h to the image instance
    img.m_width = static_cast<int>(widths[0]);
    img.m_height = static_cast<int>(heights[0]);
    // Get Channel Pitch And Sizes
    PyRocJpegUtils rocjpeg_utils;
    if (rocjpeg_utils.GetChannelPitchAndSizes(img.decode_params, img.subsampling, widths, heights, img.num_channels, img.output_image, channel_sizes)) {
        std::cerr << "ERROR: Failed to get the channel pitch and sizes" << std::endl;
        return EXIT_FAILURE;
    }
    // allocate memory for each channel
    hipError_t hip_status = hipSuccess;
    std::array<uint8_t *, ROCJPEG_MAX_COMPONENT> channels{};
    std::copy(std::begin(img.output_image.channel), std::end(img.output_image.channel), channels.begin());
    for (uint32_t i = 0; i < img.num_channels; ++i) {
        if (channels[i] != nullptr) {
            hip_status = hipFree(static_cast<void *>(channels[i]));
            if (hip_status != hipSuccess) {
                return EXIT_FAILURE;
            }
            channels[i] = nullptr;
        }
        hip_status = hipMalloc(&channels[i], static_cast<size_t>(channel_sizes[i]));
        if (hip_status != hipSuccess) {
            return EXIT_FAILURE;
        }
    }
    std::copy(channels.begin(), channels.end(), std::begin(img.output_image.channel));
    return EXIT_SUCCESS;
}
