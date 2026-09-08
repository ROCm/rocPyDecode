cmake_minimum_required(VERSION 3.15)

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

list(APPEND CMAKE_PREFIX_PATH "${ROCM_PATH}/lib/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(HIP_PLATFORM amd CACHE STRING "HIP platform")
find_package(HIP REQUIRED)
find_package(rocdecode 1.0.0 REQUIRED)
find_package(pybind11 REQUIRED)
find_package(DLPACK REQUIRED)
find_package(FFmpeg QUIET)

set(ROCPYVIDEO_UTILS "${ROCM_PATH}/share/rocdecode/utils")
if(NOT IS_DIRECTORY "${ROCPYVIDEO_UTILS}/rocvideodecode")
    message(FATAL_ERROR "rocDecode utility sources were not found under ${ROCPYVIDEO_UTILS}")
endif()
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
    src/rocdecode/roc_pyvideodecode.cpp
    "${ROCPYVIDEO_UTILS}/colorspace_kernels.cpp"
    "${ROCPYVIDEO_UTILS}/resize_kernels.cpp"
    "${ROCPYVIDEO_UTILS}/rocvideodecode/roc_video_dec.cpp")
if(ROCPYVIDEO_USE_FFMPEG)
    list(APPEND ROCPYVIDEO_SOURCES src/rocdecode/roc_pyvideodemuxer.cpp)
endif()
if(ROCPYVIDEO_ENABLE_HOST)
    list(APPEND ROCPYVIDEO_SOURCES
        src/rocdecode/roc_pydecode_test.cpp
        src/rocdecode/roc_pyvideodecodecpu.cpp)
    file(GLOB ROCPYVIDEO_FFMPEG_SOURCES "${ROCPYVIDEO_UTILS}/ffmpegvideodecode/*.cpp")
    list(APPEND ROCPYVIDEO_SOURCES ${ROCPYVIDEO_FFMPEG_SOURCES})
endif()

set(Python3_FIND_VIRTUALENV FIRST)
if(PYTHON_VERSION_SUGGESTED)
    find_package(Python3 ${PYTHON_VERSION_SUGGESTED} EXACT REQUIRED COMPONENTS Interpreter Development)
else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
endif()
string(REPLACE "." "_" ROCPYVIDEO_PYTHON_SUFFIX
    "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set(ROCPYVIDEO_TARGET "rocpydecode_${ROCPYVIDEO_PYTHON_SUFFIX}")

pybind11_add_module(${ROCPYVIDEO_TARGET} MODULE ${ROCPYVIDEO_SOURCES})
target_include_directories(${ROCPYVIDEO_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rocdecode"
    "${ROCM_PATH}/include/rocdecode"
    "${ROCPYVIDEO_UTILS}"
    "${ROCPYVIDEO_UTILS}/rocvideodecode"
    "${ROCPYVIDEO_UTILS}/ffmpegvideodecode"
    ${rocdecode_INCLUDE_DIR} ${DLPACK_INCLUDE_DIRS})
target_link_libraries(${ROCPYVIDEO_TARGET} PRIVATE hip::device rocdecode::rocdecode)
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
