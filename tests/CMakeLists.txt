################################################################################
#
# MIT License
#
# Copyright (c) 2023 Advanced Micro Devices, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
################################################################################
cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED ENHANCED_MESSAGE)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(Red "${Esc}[31m")
  set(Green "${Esc}[32m")
  set(Yellow "${Esc}[33m")
  set(White "${Esc}[37m")
endif()

# ROCM Path
if(DEFINED ENV{ROCM_PATH})
  set(ROCM_PATH $ENV{ROCM_PATH} CACHE PATH "Default ROCm installation path")
elseif(ROCM_PATH)
  message("-- INFO:ROCM_PATH Set -- ${ROCM_PATH}")
else()
  set(ROCM_PATH /opt/rocm CACHE PATH "Default ROCm installation path")
endif()
# Set AMD Clang as default compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS ON)
if(NOT DEFINED CMAKE_CXX_COMPILER)
  set(CMAKE_C_COMPILER ${ROCM_PATH}/lib/llvm/bin/amdclang)
  set(CMAKE_CXX_COMPILER ${ROCM_PATH}/lib/llvm/bin/amdclang++)
endif()

project(rocpydecode-test)

# make test with CTest
enable_testing()
include(CTest)

# add find modules
list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH}/lib/cmake)
list(APPEND CMAKE_MODULE_PATH ${ROCM_PATH}/share/rocpydecode/cmake)

# check pybind scripts
set(VIDEO_DECODE_PYBIND_SCRIPTS OFF)
if(EXISTS "${ROCM_PATH}/lib/pyRocVideoDecode")
  set(VIDEO_DECODE_PYBIND_SCRIPTS ON)
  message("-- ${White}${PROJECT_NAME}: rocPyDecode found at ${ROCM_PATH}/lib/pyRocVideoDecode${ColourReset}")
endif()
set(JPEG_DECODE_PYBIND_SCRIPTS OFF)
if(EXISTS "${ROCM_PATH}/lib/pyRocJpegDecode")
  set(JPEG_DECODE_PYBIND_SCRIPTS ON)
  message("-- ${White}${PROJECT_NAME}: rocPyJPEG found at ${ROCM_PATH}/lib/pyRocJpegDecode${ColourReset}")
endif()

# find requirements
find_package(rocdecode 1.0.0 REQUIRED)
find_package(rocjpeg 1.0.0 REQUIRED)
set(Python3_FIND_VIRTUALENV FIRST)
find_package(Python3 REQUIRED)
# rocdecode info
message("-- ${White}${PROJECT_NAME}: rocdecode found with find_package(rocdecode REQUIRED)${ColourReset}")
message("-- \t${White}rocdecode_INCLUDE_DIR -- ${rocdecode_INCLUDE_DIR}${ColourReset}")
message("-- \t${White}rocdecode_LIB_DIR -- ${rocdecode_LIB_DIR}${ColourReset}")
message("-- \t${White}rocdecode_LIBRARY -- ${rocdecode_LIBRARY}${ColourReset}")
message("-- \t${White}rocdecode_FOUND -- ${rocdecode_FOUND}${ColourReset}")
message("-- \t${White}rocdecode_VERSION -- ${rocdecode_VERSION}${ColourReset}")
message("-- \t${White}rocdecode_VERSION_MAJOR -- ${rocdecode_VERSION_MAJOR}${ColourReset}")
message("-- \t${White}rocdecode_VERSION_MINOR -- ${rocdecode_VERSION_MINOR}${ColourReset}")
message("-- \t${White}rocdecode_VERSION_PATCH -- ${rocdecode_VERSION_PATCH}${ColourReset}")
# rocjpeg info
message("-- ${White}${PROJECT_NAME}: rocjpeg found with find_package(rocjpeg REQUIRED)${ColourReset}")
message("-- \t${White}rocjpeg_INCLUDE_DIR -- ${rocjpeg_INCLUDE_DIR}${ColourReset}")
message("-- \t${White}rocjpeg_LIB_DIR -- ${rocjpeg_LIB_DIR}${ColourReset}")
message("-- \t${White}rocjpeg_LIBRARY -- ${rocjpeg_LIBRARY}${ColourReset}")
message("-- \t${White}rocjpeg_FOUND -- ${rocjpeg_FOUND}${ColourReset}")
message("-- \t${White}rocjpeg_VERSION -- ${rocjpeg_VERSION}${ColourReset}")
message("-- \t${White}rocjpeg_VERSION_MAJOR -- ${rocjpeg_VERSION_MAJOR}${ColourReset}")
message("-- \t${White}rocjpeg_VERSION_MINOR -- ${rocjpeg_VERSION_MINOR}${ColourReset}")
message("-- \t${White}rocjpeg_VERSION_PATCH -- ${rocjpeg_VERSION_PATCH}${ColourReset}")

if(NOT VIDEO_DECODE_PYBIND_SCRIPTS)
  message("-- ${Yellow}NOTE: ${PROJECT_NAME} requires the video decode bindings to be built and installed before running CTests${ColourReset}")
endif()
if(NOT JPEG_DECODE_PYBIND_SCRIPTS)
  message("-- ${Yellow}NOTE: ${PROJECT_NAME} requires the JPEG decode bindings to be built and installed before running CTests${ColourReset}")
endif()

# Tests
execute_process(COMMAND ${Python3_EXECUTABLE} -c "import torch" RESULT_VARIABLE TORCH_PYTHON_RESULT OUTPUT_QUIET ERROR_QUIET)
# add python-hip for perf tests
execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install --upgrade pip OUTPUT_VARIABLE PIP_VERSION ERROR_VARIABLE PIP_ERROR RESULT_VARIABLE PIP_RESULT)
execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install -i https://test.pypi.org/simple hip-python OUTPUT_VARIABLE PIP_VERSION ERROR_VARIABLE PIP_ERROR RESULT_VARIABLE PIP_RESULT)
# 1 - decoders test
add_test(NAME rocpydecode_test_decoders
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decoder_test.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST rocpydecode_test_decoders PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 2 - decodercpu test
add_test(NAME rocpydecode_test_decodercpu
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decodercpu_test.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST rocpydecode_test_decodercpu PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 3 - demuxer test
add_test(NAME rocpydecode_test_demuxer
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/demuxer_test.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST rocpydecode_test_demuxer PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 4 - types test
add_test(NAME rocpydecode_test_types
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/types_test.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST rocpydecode_test_types PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 5 - video_decode_python_H265 test
add_test(NAME video_decode_python_H265
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecode.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_python_H265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 6 - video_decode_perf_python_H265 test
add_test(NAME video_decode_perf_python_H265
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecodeperf.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_perf_python_H265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 7 - video_decode_rgb_python_H265 test
add_test(NAME video_decode_rgb_python_H265
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecodergb.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
  -of 3
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_rgb_python_H265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 8 - video_decode_python_H264 test
add_test(NAME video_decode_python_H264
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecode.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4 -resize 640 360 -p yes
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_python_H264 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 9 - video_decode_perf_python_H264 test
add_test(NAME video_decode_perf_python_H264
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecodeperf.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_perf_python_H264 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 10 - video_decode_python_AV1 test
add_test(NAME video_decode_python_AV1
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecode.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-AV1.mp4 -resize 640 360
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_python_AV1 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 11 - video_decode_python_AV9 test
add_test(NAME video_decode_python_AV9
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecode.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-VP9.ivf -resize 640 360
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_python_AV9 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
if(TORCH_PYTHON_RESULT EQUAL 0)
  # 12 - video_decode_python_ffmpeg(torch) test
  add_test(NAME video_decode_python_ffmpeg_torch
    COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecodetorch_cpu_backend.py
    -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4 -m 2
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST video_decode_python_ffmpeg_torch PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
endif()
# 13 - video_decode_python_ffmpeg test
add_test(NAME video_decode_python_ffmpeg
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocVideoDecode/samples/rocdecode/videodecode_cpu_backend.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4 -m 2
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST video_decode_python_ffmpeg PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 14 - all_other_decoder_apis test
add_test(NAME all_other_decoder_apis
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decoder_api_test.py
  -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST all_other_decoder_apis PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
# 15 - jpeg_decode_single_file_test
if(TORCH_PYTHON_RESULT EQUAL 0)
  add_test(NAME jpeg_decode_python
    COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocJpegDecode/samples/rocjpeg/jpegdecode.py
    -i ${ROCM_PATH}/share/rocjpeg/images/mug_420.jpg
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  set_property(TEST jpeg_decode_python PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
endif(TORCH_PYTHON_RESULT EQUAL 0)
# 16 - jpegdecodebatched_test
add_test(NAME jpeg_decode_batched_python
  COMMAND ${Python3_EXECUTABLE} ${ROCM_PATH}/lib/pyRocJpegDecode/samples/rocjpeg/jpegdecodebatched.py
  -i ${ROCM_PATH}/share/rocjpeg/images/
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_property(TEST jpeg_decode_batched_python PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
