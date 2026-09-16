cmake_minimum_required(VERSION 3.18)

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

set(_rocpyvideo_target "")
if(BUILD_VIDEO_DECODE)
    add_subdirectory(videoDecode)
    get_directory_property(_rocpyvideo_target DIRECTORY videoDecode DEFINITION ROCPYVIDEO_TARGET)
endif()

if(BUILD_JPEG_DECODE)
    add_subdirectory(jpegDecode)
endif()

if(NOT _rocpyvideo_target AND NOT BUILD_JPEG_DECODE)
    message(WARNING "No rocPyDecode components remain; nothing will be built.")
endif()

if(BUILD_TESTING)
    add_subdirectory(cmake/tests)
endif()
