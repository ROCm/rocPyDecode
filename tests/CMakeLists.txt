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

# Python is required to locate pybind outputs and run the tests
set(Python3_FIND_VIRTUALENV FIRST)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(PY_VERSION_SUFFIX "${Python3_VERSION_MAJOR}_${Python3_VERSION_MINOR}")

# make test with CTest
enable_testing()
include(CTest)

# add find modules
list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH}/lib/cmake)
list(APPEND CMAKE_MODULE_PATH ${ROCM_PATH}/share/rocpydecode/cmake)

# Default (installed) locations
set(ROCPYDECODE_LIB_DIR "${ROCM_PATH}/lib")
set(ROCPYDECODE_PY_DIR "${ROCM_PATH}/lib/pyRocVideoDecode")
set(ROCPYJPEG_LIB_DIR "${ROCM_PATH}/lib")
set(ROCPYJPEG_PY_DIR "${ROCM_PATH}/lib/pyRocJpegDecode")

# In-tree build locations (match pybind target layout)
set(ROCPYDECODE_BUILD_LIB_DIR "${CMAKE_BINARY_DIR}/rocpydecode/lib")
set(ROCPYDECODE_BUILD_PY_DIR "${CMAKE_BINARY_DIR}/rocpydecode_${PY_VERSION_SUFFIX}/pyRocVideoDecode")
set(ROCPYJPEG_BUILD_LIB_DIR "${CMAKE_BINARY_DIR}/rocpyjpegdecode/lib")
set(ROCPYJPEG_BUILD_PY_DIR "${CMAKE_BINARY_DIR}/rocpyjpegdecode_${PY_VERSION_SUFFIX}/pyRocJpegDecode")

# check installed or build-tree pybind bindings
set(VIDEO_DECODE_BINDINGS_INSTALLED OFF)
if(EXISTS "${ROCPYDECODE_PY_DIR}")
  set(VIDEO_DECODE_BINDINGS_INSTALLED ON)
  message("-- ${White}${PROJECT_NAME}: rocPyDecode found at ${ROCPYDECODE_PY_DIR}${ColourReset}")
endif()
if(VIDEO_DECODE_BINDINGS_INSTALLED)
  file(GLOB ROCPYDECODE_PYMODULES "${ROCPYDECODE_LIB_DIR}/rocpydecode*.so" "${ROCPYDECODE_LIB_DIR}/rocpydecode*.pyd")
  if(NOT ROCPYDECODE_PYMODULES)
    set(VIDEO_DECODE_BINDINGS_INSTALLED OFF)
    message("-- ${Yellow}${PROJECT_NAME}: rocPyDecode bindings directory present but shared object not found in ${ROCPYDECODE_LIB_DIR}; will try build-tree outputs.${ColourReset}")
  endif()
endif()
if(NOT VIDEO_DECODE_BINDINGS_INSTALLED)
  file(GLOB ROCPYDECODE_PYMODULES "${ROCPYDECODE_BUILD_LIB_DIR}/rocpydecode*.so" "${ROCPYDECODE_BUILD_LIB_DIR}/rocpydecode*.pyd")
  if(ROCPYDECODE_PYMODULES AND EXISTS "${ROCPYDECODE_BUILD_PY_DIR}")
    set(VIDEO_DECODE_BINDINGS_INSTALLED ON)
    set(ROCPYDECODE_LIB_DIR "${ROCPYDECODE_BUILD_LIB_DIR}")
    set(ROCPYDECODE_PY_DIR "${ROCPYDECODE_BUILD_PY_DIR}")
    message("-- ${White}${PROJECT_NAME}: using in-tree rocPyDecode build outputs at ${ROCPYDECODE_LIB_DIR}${ColourReset}")
  endif()
endif()

set(JPEG_DECODE_PYBIND_SCRIPTS OFF)
if(EXISTS "${ROCPYJPEG_PY_DIR}")
  set(JPEG_DECODE_PYBIND_SCRIPTS ON)
  message("-- ${White}${PROJECT_NAME}: rocPyJPEG found at ${ROCPYJPEG_PY_DIR}${ColourReset}")
endif()
if(JPEG_DECODE_PYBIND_SCRIPTS)
  file(GLOB ROCPYJPEG_PYMODULES "${ROCPYJPEG_LIB_DIR}/rocpyjpegdecode*.so" "${ROCPYJPEG_LIB_DIR}/rocpyjpegdecode*.pyd")
  if(NOT ROCPYJPEG_PYMODULES)
    set(JPEG_DECODE_PYBIND_SCRIPTS OFF)
    message("-- ${Yellow}${PROJECT_NAME}: rocPyJPEG bindings directory present but shared object not found in ${ROCPYJPEG_LIB_DIR}; will try build-tree outputs.${ColourReset}")
  endif()
endif()
if(NOT JPEG_DECODE_PYBIND_SCRIPTS)
  file(GLOB ROCPYJPEG_PYMODULES "${ROCPYJPEG_BUILD_LIB_DIR}/rocpyjpegdecode*.so" "${ROCPYJPEG_BUILD_LIB_DIR}/rocpyjpegdecode*.pyd")
  if(ROCPYJPEG_PYMODULES AND EXISTS "${ROCPYJPEG_BUILD_PY_DIR}")
    set(JPEG_DECODE_PYBIND_SCRIPTS ON)
    set(ROCPYJPEG_LIB_DIR "${ROCPYJPEG_BUILD_LIB_DIR}")
    set(ROCPYJPEG_PY_DIR "${ROCPYJPEG_BUILD_PY_DIR}")
    message("-- ${White}${PROJECT_NAME}: using in-tree rocPyJPEG build outputs at ${ROCPYJPEG_LIB_DIR}${ColourReset}")
  endif()
endif()

# fallback sample/script locations and PYTHONPATH for tests
set(ROCPYDECODE_SAMPLE_DIR "${ROCPYDECODE_PY_DIR}/samples/rocdecode")
if(NOT EXISTS "${ROCPYDECODE_SAMPLE_DIR}")
  set(ROCPYDECODE_SAMPLE_DIR "${CMAKE_SOURCE_DIR}/samples/rocdecode")
endif()
get_filename_component(ROCPYDECODE_PY_PARENT "${ROCPYDECODE_PY_DIR}" DIRECTORY)
set(_ROCPYDECODE_PATHS "${ROCPYDECODE_LIB_DIR}:${ROCPYDECODE_PY_PARENT}")
if(DEFINED ENV{PYTHONPATH} AND NOT "$ENV{PYTHONPATH}" STREQUAL "")
  set(ROCPYDECODE_PYTHONPATH "$ENV{PYTHONPATH}:${_ROCPYDECODE_PATHS}")
else()
  set(ROCPYDECODE_PYTHONPATH "${_ROCPYDECODE_PATHS}")
endif()

set(ROCPYJPEG_SAMPLE_DIR "${ROCPYJPEG_PY_DIR}/samples/rocjpeg")
if(NOT EXISTS "${ROCPYJPEG_SAMPLE_DIR}")
  set(ROCPYJPEG_SAMPLE_DIR "${CMAKE_SOURCE_DIR}/samples/rocjpeg")
endif()
get_filename_component(ROCPYJPEG_PY_PARENT "${ROCPYJPEG_PY_DIR}" DIRECTORY)
set(_ROCPYJPEG_PATHS "${ROCPYJPEG_LIB_DIR}:${ROCPYJPEG_PY_PARENT}")
if(DEFINED ENV{PYTHONPATH} AND NOT "$ENV{PYTHONPATH}" STREQUAL "")
  set(ROCPYJPEG_PYTHONPATH "$ENV{PYTHONPATH}:${_ROCPYJPEG_PATHS}")
else()
  set(ROCPYJPEG_PYTHONPATH "${_ROCPYJPEG_PATHS}")
endif()

# find requirements (optional to allow skipping tests)
find_package(rocdecode 1.0.0 QUIET)
find_package(rocjpeg 1.0.0 QUIET)
find_library(rocdecode_HOST_LIBRARY NAMES rocdecodehost rocdecode-host PATHS ${ROCM_PATH}/lib)
set(RUN_ROCPYDECODE_TESTS OFF)
set(RUN_ROCPYJPEG_TESTS OFF)
set(RUN_ROCPYDECODE_HOST_TESTS OFF)

if(rocdecode_FOUND AND VIDEO_DECODE_BINDINGS_INSTALLED)
  set(RUN_ROCPYDECODE_TESTS ON)
  if(rocdecode_HOST_LIBRARY)
    set(RUN_ROCPYDECODE_HOST_TESTS ON)
  else()
    message("-- ${Yellow}${PROJECT_NAME}: skipping rocPyDecode CPU backend tests; rocdecode host library not found at ${ROCM_PATH}/lib.${ColourReset}")
  endif()
  message("-- ${White}${PROJECT_NAME}: enabling rocPyDecode tests${ColourReset}")
elseif(NOT rocdecode_FOUND)
  message("-- ${Yellow}${PROJECT_NAME}: skipping rocPyDecode tests; rocdecode not found.${ColourReset}")
else()
  message("-- ${Yellow}${PROJECT_NAME}: skipping rocPyDecode tests; rocPyDecode bindings not found in install path or build tree${ColourReset}")
endif()

if(rocjpeg_FOUND AND JPEG_DECODE_PYBIND_SCRIPTS)
  set(RUN_ROCPYJPEG_TESTS ON)
  message("-- ${White}${PROJECT_NAME}: enabling rocPyJPEG tests${ColourReset}")
elseif(NOT rocjpeg_FOUND)
  message("-- ${Yellow}${PROJECT_NAME}: skipping rocPyJPEG tests; rocjpeg not found.${ColourReset}")
else()
  message("-- ${Yellow}${PROJECT_NAME}: skipping rocPyJPEG tests; rocPyJPEG bindings not found in install path or build tree${ColourReset}")
endif()

message("-- ${White}${PROJECT_NAME}: summary RUN_ROCPYDECODE_TESTS=${RUN_ROCPYDECODE_TESTS} RUN_ROCPYDECODE_HOST_TESTS=${RUN_ROCPYDECODE_HOST_TESTS} RUN_ROCPYJPEG_TESTS=${RUN_ROCPYJPEG_TESTS}${ColourReset}")

if(NOT VIDEO_DECODE_BINDINGS_INSTALLED)
  message("-- ${Yellow}NOTE: ${PROJECT_NAME} requires the video decode bindings to be built (build tree or installed) before running CTests${ColourReset}")
endif()
if(NOT JPEG_DECODE_PYBIND_SCRIPTS)
  message("-- ${Yellow}NOTE: ${PROJECT_NAME} requires the JPEG decode bindings to be built (build tree or installed) before running CTests${ColourReset}")
endif()

# Check if lib/rocm_sysdeps/lib exists in the ROCm path which indicates ROCm installation via TheRock
set(USING_THE_ROCK OFF)
if(EXISTS "${ROCM_PATH}/lib/rocm_sysdeps/lib")
  set(USING_THE_ROCK ON)
endif()

if(RUN_ROCPYDECODE_TESTS OR RUN_ROCPYJPEG_TESTS)
  # Tests
  execute_process(COMMAND ${Python3_EXECUTABLE} -c "import torch" RESULT_VARIABLE TORCH_PYTHON_RESULT OUTPUT_QUIET ERROR_QUIET)
  # add python-hip for perf tests
  execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install --upgrade pip OUTPUT_VARIABLE PIP_VERSION ERROR_VARIABLE PIP_ERROR RESULT_VARIABLE PIP_RESULT)
  execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install -i https://test.pypi.org/simple hip-python OUTPUT_VARIABLE PIP_VERSION ERROR_VARIABLE PIP_ERROR RESULT_VARIABLE PIP_RESULT)
else()
  set(TORCH_PYTHON_RESULT 1)
endif()

if(RUN_ROCPYDECODE_TESTS)
  if(ROCPYDECODE_USE_FFMPEG)
    # 1 - decoders test (demux / FFmpeg dependent)
    add_test(NAME rocpydecode_test_decoders
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decoder_test.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST rocpydecode_test_decoders PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    # 2 - decodercpu test
    if(RUN_ROCPYDECODE_HOST_TESTS)
      add_test(NAME rocpydecode_test_decodercpu
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decodercpu_test.py
        -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
      set_property(TEST rocpydecode_test_decodercpu PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    endif()
    # 3 - demuxer test
    add_test(NAME rocpydecode_test_demuxer
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/demuxer_test.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST rocpydecode_test_demuxer PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
  else()
    message("-- ${Yellow}${PROJECT_NAME}: FFmpeg not available; skipping decoder/demuxer/CPU sample tests.${ColourReset}")
  endif()
  # 4 - types test
  add_test(NAME rocpydecode_test_types
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/types_test.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST rocpydecode_test_types PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
  if(ROCPYDECODE_USE_FFMPEG)
    # 5 - video_decode_python_H265 test
    add_test(NAME video_decode_python_H265
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecode.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST video_decode_python_H265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    # 6 - video_decode_perf_python_H265 test
    add_test(NAME video_decode_perf_python_H265
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecodeperf.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    if(USING_THE_ROCK)
      set_property(TEST video_decode_perf_python_H265 PROPERTY ENVIRONMENT
        "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$ENV{PYTHONPATH}"
        "LD_LIBRARY_PATH=${ROCM_PATH}/lib:$ENV{LD_LIBRARY_PATH}")
    else()
      set_property(TEST video_decode_perf_python_H265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    endif()
    # 7 - video_decode_rgb_python_H265 test
    add_test(NAME video_decode_rgb_python_H265
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecodergb.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
      -of 3
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST video_decode_rgb_python_H265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    # 8 - video_decode_python_H264 test
    add_test(NAME video_decode_python_H264
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecode.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4 -resize 640 360 -p yes
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST video_decode_python_H264 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    # 9 - video_decode_perf_python_H264 test
    add_test(NAME video_decode_perf_python_H264
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecodeperf.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    if(USING_THE_ROCK)
      set_property(TEST video_decode_perf_python_H264 PROPERTY ENVIRONMENT
        "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$ENV{PYTHONPATH}"
        "LD_LIBRARY_PATH=${ROCM_PATH}/lib:$ENV{LD_LIBRARY_PATH}")
    else()
      set_property(TEST video_decode_perf_python_H264 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    endif()
    # 10 - video_decode_python_AV1 test
    add_test(NAME video_decode_python_AV1
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecode.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-AV1.mp4 -resize 640 360
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST video_decode_python_AV1 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    # 11 - video_decode_python_AV9 test
    add_test(NAME video_decode_python_AV9
      COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecode.py
      -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-VP9.ivf -resize 640 360
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_property(TEST video_decode_python_AV9 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    if(TORCH_PYTHON_RESULT EQUAL 0 AND RUN_ROCPYDECODE_HOST_TESTS)
      # 12 - video_decode_python_ffmpeg(torch) test
      add_test(NAME video_decode_python_ffmpeg_torch
        COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecodetorch_cpu_backend.py
        -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4 -m 2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
      set_property(TEST video_decode_python_ffmpeg_torch PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    endif()
    # 13 - video_decode_python_ffmpeg test
    if(RUN_ROCPYDECODE_HOST_TESTS)
      add_test(NAME video_decode_python_ffmpeg
        COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecode_cpu_backend.py
        -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4 -m 2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
      set_property(TEST video_decode_python_ffmpeg PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
    endif()
  endif()
  # 14 - video_decode_raw_python (Annex-B raw bitstream)
  add_test(NAME video_decode_raw_python
    COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecoderaw.py
    -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.264
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST video_decode_raw_python PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
  # 15 - video_decode_raw_python_h265 (Annex-B raw HEVC bitstream)
  add_test(NAME video_decode_raw_python_h265
    COMMAND ${Python3_EXECUTABLE} ${ROCPYDECODE_SAMPLE_DIR}/videodecoderaw.py
    -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.265
    --codec h265
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST video_decode_raw_python_h265 PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
  # 16 - all_other_decoder_apis test
  add_test(NAME all_other_decoder_apis
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decoder_api_test.py
    -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST all_other_decoder_apis PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
endif()

if(RUN_ROCPYJPEG_TESTS)
  # 17 - jpeg_decode_single_file_test
  if(TORCH_PYTHON_RESULT EQUAL 0)
    add_test(NAME jpeg_decode_python
      COMMAND ${Python3_EXECUTABLE} ${ROCPYJPEG_SAMPLE_DIR}/jpegdecode.py
      -i ${ROCM_PATH}/share/rocjpeg/images/mug_420.jpg
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    if(USING_THE_ROCK)
      set_property(TEST jpeg_decode_python PROPERTY ENVIRONMENT
        "PYTHONPATH=${ROCPYJPEG_PYTHONPATH}:$ENV{PYTHONPATH}"
        "LD_LIBRARY_PATH=${ROCM_PATH}/lib:$ENV{LD_LIBRARY_PATH}")
    else()
      set_property(TEST jpeg_decode_python PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYJPEG_PYTHONPATH}:$PYTHONPATH")
    endif()
  endif(TORCH_PYTHON_RESULT EQUAL 0)
  # 18 - jpegdecodebatched_test
  add_test(NAME jpeg_decode_batched_python
    COMMAND ${Python3_EXECUTABLE} ${ROCPYJPEG_SAMPLE_DIR}/jpegdecodebatched.py
    -i ${ROCM_PATH}/share/rocjpeg/images/
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  if(USING_THE_ROCK)
    set_property(TEST jpeg_decode_batched_python PROPERTY ENVIRONMENT
      "PYTHONPATH=${ROCPYJPEG_PYTHONPATH}:$ENV{PYTHONPATH}"
      "LD_LIBRARY_PATH=${ROCM_PATH}/lib:$ENV{LD_LIBRARY_PATH}")
  else()
    set_property(TEST jpeg_decode_batched_python PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYJPEG_PYTHONPATH}:$PYTHONPATH")
  endif()
endif()

if(RUN_ROCPYDECODE_TESTS AND ROCPYDECODE_USE_FFMPEG)
  # 19 - decoder rgb dlpack test
  add_test(NAME rocpydecode_test_decoder_rgb_dlpack
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decoder_rgb_dlpack_test.py
    -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST rocpydecode_test_decoder_rgb_dlpack PROPERTY ENVIRONMENT "PYTHONPATH=${ROCPYDECODE_PYTHONPATH}:$PYTHONPATH")
endif()
