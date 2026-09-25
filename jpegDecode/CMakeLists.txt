cmake_minimum_required(VERSION 3.18)

if(DEFINED ENV{ROCM_PATH})
    set(ROCM_PATH "$ENV{ROCM_PATH}" CACHE PATH "ROCm installation path")
elseif(NOT ROCM_PATH)
    set(ROCM_PATH /opt/rocm CACHE PATH "ROCm installation path")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${ROCM_PATH}/lib/llvm/bin/amdclang++")
endif()

project(rocPyJpegDecode VERSION 1.0.0 LANGUAGES CXX)
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
find_package(rocjpeg 1.0.0 REQUIRED)

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

string(REPLACE "." "_" ROCPYJPEG_PYTHON_SUFFIX
    "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set(ROCPYJPEG_TARGET "rocpyjpegdecode_${ROCPYJPEG_PYTHON_SUFFIX}")

file(GLOB ROCPYJPEG_SOURCES CONFIGURE_DEPENDS src/rocjpeg/*.cpp)
pybind11_add_module(${ROCPYJPEG_TARGET} MODULE
    src/roc_pybuffer.cpp src/roc_pydlpack.cpp ${ROCPYJPEG_SOURCES})
target_include_directories(${ROCPYJPEG_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rocjpeg"
    ${rocjpeg_INCLUDE_DIR})
target_link_libraries(${ROCPYJPEG_TARGET} PRIVATE dlpack::dlpack hip::device rocjpeg::rocjpeg)

set(ROCPYJPEG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ROCPYJPEG_TARGET}")
file(COPY pyRocJpegDecode DESTINATION "${ROCPYJPEG_OUTPUT_DIR}")
set_target_properties(${ROCPYJPEG_TARGET} PROPERTIES PREFIX "" OUTPUT_NAME rocpyjpegdecode
    LIBRARY_OUTPUT_DIRECTORY "${ROCPYJPEG_OUTPUT_DIR}/lib")
if(EXISTS "${ROCM_PATH}/lib/rocm_sysdeps/lib")
    set_target_properties(${ROCPYJPEG_TARGET} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN;$ORIGIN/rocm_sysdeps/lib")
endif()

install(TARGETS ${ROCPYJPEG_TARGET} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY pyRocJpegDecode DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY samples/rocjpeg DESTINATION ${CMAKE_INSTALL_DATADIR}/rocpyjpegdecode/samples)
install(FILES LICENSE.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/rocpyjpegdecode)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
message(STATUS "rocPyJpegDecode target: ${ROCPYJPEG_TARGET}")
