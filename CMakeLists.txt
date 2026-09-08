cmake_minimum_required(VERSION 3.15)

if(DEFINED ENV{ROCM_PATH})
    set(ROCM_PATH "$ENV{ROCM_PATH}" CACHE PATH "ROCm installation path")
elseif(NOT ROCM_PATH)
    set(ROCM_PATH /opt/rocm CACHE PATH "ROCm installation path")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${ROCM_PATH}/lib/llvm/bin/amdclang++")
endif()

set(VERSION "1.0.0")
project(rocPyDecode VERSION ${VERSION} LANGUAGES CXX)

option(BUILD_VIDEO_DECODE "Build the rocDecode Python bindings" ON)
option(BUILD_JPEG_DECODE "Build the rocJPEG Python bindings" ON)

include(CTest)

if(BUILD_VIDEO_DECODE)
    add_subdirectory(videoDecode)
endif()

if(BUILD_JPEG_DECODE)
    add_subdirectory(jpegDecode)
endif()

if(NOT BUILD_VIDEO_DECODE AND NOT BUILD_JPEG_DECODE)
    message(WARNING "Both rocPyDecode components are disabled; no targets will be built.")
endif()
