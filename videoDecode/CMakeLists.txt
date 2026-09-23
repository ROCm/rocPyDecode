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
find_package(HIP REQUIRED)
find_package(rocdecode 1.0.0 REQUIRED)
set(ROCPYVIDEO_SOURCES
    src/roc_pybuffer.cpp
    src/roc_pydlpack.cpp
    src/rocdecode/roc_pydecode.cpp
    src/rocdecode/roc_pycpu_surface.cpp
    src/rocdecode/roc_pydecode_test.cpp
    src/rocdecode/roc_pyvideodecode.cpp)
set(ROCPYVIDEO_SDK_SOURCES
    "${ROCPYVIDEO_UTILS}/colorspace_kernels.cpp"
    "${ROCPYVIDEO_UTILS}/rocvideodecode/roc_video_dec.cpp")
set(Python3_FIND_VIRTUALENV FIRST)
if(PYTHON_VERSION_SUGGESTED)
    find_package(Python3 ${PYTHON_VERSION_SUGGESTED} EXACT REQUIRED COMPONENTS Interpreter Development.Module)
else()
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter Development.Module)
endif()
if(Python3_VERSION VERSION_LESS "3.11")
    message(FATAL_ERROR "rocPyDecode requires Python 3.11 or newer")
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
    ${rocdecode_INCLUDE_DIR})
target_link_libraries(${ROCPYVIDEO_TARGET} PRIVATE dlpack::dlpack hip::device rocdecode::rocdecode)
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
message(STATUS "rocPyVideoDecode demuxing and CPU decoding: optional PyAV runtime")
install(FILES requirements.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/rocpydecode)
