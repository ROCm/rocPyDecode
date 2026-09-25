cmake_minimum_required(VERSION 3.18)

if(DEFINED ENV{ROCM_PATH})
    set(ROCM_PATH "$ENV{ROCM_PATH}" CACHE PATH "ROCm installation path")
elseif(NOT ROCM_PATH)
    set(ROCM_PATH /opt/rocm CACHE PATH "ROCm installation path")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${ROCM_PATH}/lib/llvm/bin/amdclang++")
endif()

project(rocPyVideoDecode VERSION 1.0.0 LANGUAGES CXX)
include(CTest)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${ROCM_PATH}" CACHE PATH "Installation path" FORCE)
endif()

# Validate utility sources before discovering video-only dependencies. An explicit
# override is authoritative: never fall back to another SDK's utility sources.
set(ROCPYVIDEO_UTILS_DIR "${ROCM_PATH}/share/rocdecode/utils" CACHE PATH
    "Directory containing compatible rocDecode utility sources")
set(ROCPYVIDEO_UTILS "${ROCPYVIDEO_UTILS_DIR}")
set(_rocpyvideo_missing_utils)
foreach(_rocpyvideo_file IN ITEMS
        rocvideodecode/roc_video_dec.cpp
        rocvideodecode/roc_video_dec.h
        colorspace_kernels.cpp
        colorspace_kernels.h
        resize_kernels.h
        video_post_process.h)
    if(NOT EXISTS "${ROCPYVIDEO_UTILS}/${_rocpyvideo_file}" OR
       IS_DIRECTORY "${ROCPYVIDEO_UTILS}/${_rocpyvideo_file}")
        list(APPEND _rocpyvideo_missing_utils "${_rocpyvideo_file}")
    endif()
endforeach()
if(_rocpyvideo_missing_utils)
    string(REPLACE ";" "\n  " _rocpyvideo_missing_text "${_rocpyvideo_missing_utils}")
    message(WARNING
        "Skipping rocPyVideoDecode: incomplete rocDecode utility directory:\n"
        "  ${ROCPYVIDEO_UTILS}\nMissing required files:\n  ${_rocpyvideo_missing_text}\n"
        "Set ROCPYVIDEO_UTILS_DIR to a compatible, complete utility directory. "
        "No alternative directory will be searched. Video targets, tests, and "
        "installation rules will not be generated.")
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        message(WARNING "No rocPyDecode components remain; nothing will be built.")
    endif()
    return()
endif()

list(APPEND CMAKE_PREFIX_PATH "${ROCM_PATH}/lib/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(HIP_PLATFORM amd CACHE STRING "HIP platform")
# Preserve HIP's legacy target override when GPU_TARGETS is not supplied.
if(NOT DEFINED GPU_TARGETS AND DEFINED AMDGPU_TARGETS)
    set(GPU_TARGETS "${AMDGPU_TARGETS}" CACHE STRING "AMD GPU targets to compile for")
endif()

# Select GPU targets before HIP discovery, without probing local devices.
if(NOT DEFINED GPU_TARGETS)
    set(GPU_TARGETS
        gfx950 gfx942 gfx90a gfx908 gfx1250
        gfx1201 gfx1200 gfx1153 gfx1152 gfx1151 gfx1150
        gfx1103 gfx1102 gfx1101 gfx1100
        gfx1036 gfx1035 gfx1034 gfx1033 gfx1032 gfx1031 gfx1030
        gfx1012 gfx1011 gfx1010
        CACHE STRING "AMD GPU targets to compile for")
endif()

if(NOT GPU_TARGETS)
    message(FATAL_ERROR "GPU_TARGETS must contain at least one GPU architecture, for example -DGPU_TARGETS=gfx942")
endif()

# HIP caches this derived list separately; refresh it when targets change.
set(GPU_BUILD_TARGETS "${GPU_TARGETS}" CACHE STRING "GPU targets to compile for" FORCE)

message(STATUS "${PROJECT_NAME} GPU targets: ${GPU_TARGETS}")

find_package(HIP REQUIRED)
find_package(rocdecode 1.0.0 REQUIRED)
find_package(FFmpeg QUIET)

find_library(ROCPYVIDEO_HOST_LIBRARY NAMES rocdecode-host rocdecodehost
    PATHS "${ROCM_PATH}/lib" NO_DEFAULT_PATH)
set(ROCPYVIDEO_USE_FFMPEG OFF)
if(FFMPEG_FOUND AND EXISTS "${ROCPYVIDEO_UTILS}/video_demuxer.h")
    set(ROCPYVIDEO_USE_FFMPEG ON)
endif()

set(ROCPYVIDEO_ENABLE_HOST OFF)
if(ROCPYVIDEO_USE_FFMPEG AND ROCPYVIDEO_HOST_LIBRARY AND
   EXISTS "${ROCPYVIDEO_UTILS}/ffmpegvideodecode/ffmpeg_video_dec.h")
    set(ROCPYVIDEO_ENABLE_HOST ON)
endif()

set(ROCPYVIDEO_SOURCES
    src/roc_pybuffer.cpp
    src/roc_pydlpack.cpp
    src/rocdecode/roc_pydecode.cpp
    src/rocdecode/roc_pyvideodecode.cpp)
set(ROCPYVIDEO_SDK_SOURCES
    "${ROCPYVIDEO_UTILS}/colorspace_kernels.cpp"
    "${ROCPYVIDEO_UTILS}/rocvideodecode/roc_video_dec.cpp")
if(ROCPYVIDEO_USE_FFMPEG)
    list(APPEND ROCPYVIDEO_SOURCES src/rocdecode/roc_pyvideodemuxer.cpp)
endif()
if(ROCPYVIDEO_ENABLE_HOST)
    list(APPEND ROCPYVIDEO_SOURCES
        src/rocdecode/roc_pydecode_test.cpp
        src/rocdecode/roc_pyvideodecodecpu.cpp)
    file(GLOB ROCPYVIDEO_FFMPEG_SOURCES "${ROCPYVIDEO_UTILS}/ffmpegvideodecode/*.cpp")
    list(APPEND ROCPYVIDEO_SDK_SOURCES ${ROCPYVIDEO_FFMPEG_SOURCES})
endif()

set(Python3_FIND_VIRTUALENV FIRST)
if(PYTHON_VERSION_SUGGESTED)
    find_package(Python3 ${PYTHON_VERSION_SUGGESTED} EXACT REQUIRED COMPONENTS Interpreter Development.Module)
else()
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter Development.Module)
endif()
if(Python3_VERSION VERSION_LESS "3.9")
    message(FATAL_ERROR "Bundled pybind11 3.1.0 requires Python 3.9 or newer")
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/third_party/Dependencies.cmake")

string(REPLACE "." "_" ROCPYVIDEO_PYTHON_SUFFIX
    "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set(ROCPYVIDEO_TARGET "rocpydecode_${ROCPYVIDEO_PYTHON_SUFFIX}")

pybind11_add_module(${ROCPYVIDEO_TARGET} MODULE
    ${ROCPYVIDEO_SOURCES} ${ROCPYVIDEO_SDK_SOURCES})
target_include_directories(${ROCPYVIDEO_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rocdecode")

# SDK utility warnings remain visible but do not inherit -Werror. SDK headers
# are SYSTEM includes so their diagnostics do not fail project translation units.
# Project sources still honor the caller's warning/error flags.
set_property(SOURCE ${ROCPYVIDEO_SDK_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-error>"
    "$<$<CXX_COMPILER_ID:MSVC>:/WX->")
message(STATUS "rocDecode SDK source warnings remain visible and non-fatal; "
    "SDK headers use SYSTEM includes. SDK warnings are not fixed.")
target_include_directories(${ROCPYVIDEO_TARGET} SYSTEM PRIVATE
    "${ROCM_PATH}/include/rocdecode"
    "${ROCPYVIDEO_UTILS}"
    "${ROCPYVIDEO_UTILS}/rocvideodecode"
    "${ROCPYVIDEO_UTILS}/ffmpegvideodecode"
    ${rocdecode_INCLUDE_DIR})
target_link_libraries(${ROCPYVIDEO_TARGET} PRIVATE dlpack::dlpack hip::device rocdecode::rocdecode)
target_compile_definitions(${ROCPYVIDEO_TARGET} PRIVATE
    ROCPYDECODE_ENABLE_HOST=$<BOOL:${ROCPYVIDEO_ENABLE_HOST}>
    ROCPYDECODE_USE_FFMPEG=$<BOOL:${ROCPYVIDEO_USE_FFMPEG}>)
if(ROCPYVIDEO_USE_FFMPEG)
    target_include_directories(${ROCPYVIDEO_TARGET} PRIVATE
        ${AVUTIL_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR})
    target_link_libraries(${ROCPYVIDEO_TARGET} PRIVATE ${FFMPEG_LIBRARIES})
    if(_FFMPEG_AVCODEC_VERSION VERSION_LESS_EQUAL 58.134.100)
        target_compile_definitions(${ROCPYVIDEO_TARGET} PRIVATE USE_AVCODEC_GREATER_THAN_58_134=0)
    else()
        target_compile_definitions(${ROCPYVIDEO_TARGET} PRIVATE USE_AVCODEC_GREATER_THAN_58_134=1)
    endif()
endif()
if(ROCPYVIDEO_ENABLE_HOST)
    target_link_libraries(${ROCPYVIDEO_TARGET} PRIVATE ${ROCPYVIDEO_HOST_LIBRARY})
endif()

set(ROCPYVIDEO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ROCPYVIDEO_TARGET}")
file(COPY pyRocVideoDecode DESTINATION "${ROCPYVIDEO_OUTPUT_DIR}")
set_target_properties(${ROCPYVIDEO_TARGET} PROPERTIES PREFIX "" OUTPUT_NAME rocpydecode
    LIBRARY_OUTPUT_DIRECTORY "${ROCPYVIDEO_OUTPUT_DIR}/lib")
if(EXISTS "${ROCM_PATH}/lib/rocm_sysdeps/lib")
    set_target_properties(${ROCPYVIDEO_TARGET} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN;$ORIGIN/rocm_sysdeps/lib")
endif()

install(TARGETS ${ROCPYVIDEO_TARGET} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY pyRocVideoDecode DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY samples/rocdecode DESTINATION ${CMAKE_INSTALL_DATADIR}/rocpydecode/samples)
install(FILES LICENSE.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/rocpydecode)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
message(STATUS "rocPyVideoDecode target: ${ROCPYVIDEO_TARGET}")
message(STATUS "rocPyVideoDecode FFmpeg support: ${ROCPYVIDEO_USE_FFMPEG}")
message(STATUS "rocPyVideoDecode CPU backend support: ${ROCPYVIDEO_ENABLE_HOST}")
