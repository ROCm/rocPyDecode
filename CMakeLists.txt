################################################################################
# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to dec
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

cmake_minimum_required(VERSION 3.12)

# ROCM Path
if(DEFINED ENV{ROCM_PATH})
    set(ROCM_PATH $ENV{ROCM_PATH} CACHE PATH "Default ROCm installation path")
elseif(ROCM_PATH)
    message("-- INFO:ROCM_PATH Set -- ${ROCM_PATH}")
else()
    set(ROCM_PATH /opt/rocm CACHE PATH "Default ROCm installation path")
endif()
# Set AMD Clang as default compiler
if (NOT DEFINED CMAKE_CXX_COMPILER AND EXISTS "${ROCM_PATH}/bin/amdclang++")
    set(CMAKE_C_COMPILER ${ROCM_PATH}/bin/amdclang)
    set(CMAKE_CXX_COMPILER ${ROCM_PATH}/bin/amdclang++)
endif()

set(VERSION "0.3.0")
set(CMAKE_CXX_STANDARD 17)

# Set Project Version and Language
project(rocPyDecode VERSION ${VERSION} LANGUAGES CXX)

set(TARGET_NAME rocPyDecode)

# rocPyDecode build type
set(DEFAULT_BUILD_TYPE "Release")
set(ENHANCED_MESSAGE ON)

if(ENHANCED_MESSAGE)
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(Red "${Esc}[31m")
    set(Green "${Esc}[32m")
    set(Yellow "${Esc}[33m")
    set(Blue "${Esc}[34m")
    set(BoldBlue "${Esc}[1;34m")
    set(Magenta "${Esc}[35m")
    set(Cyan "${Esc}[36m")
    set(White "${Esc}[37m")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH}/hip ${ROCM_PATH})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "rocPyDecode Default Build Type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    # -O0 -- Don't Optimize output file
    # -gdwarf-4  -- generate debugging information, dwarf-4 for making valgrind work
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -gdwarf-4")
else()
    # -O3       -- Optimize output file 
    # -DNDEBUG  -- turn off asserts 
    # -fPIC     -- Generate position-independent code if possible
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -fPIC")
endif()

message("-- ${Cyan}     -D ROCPYDECODE_AMDGPU_TARGET=${ROCPYDECODE_AMDGPU_TARGET} [Build for specific AMD GPUs]${ColourReset}")

# set amdgpu targets with vcn support
option(ROCPYDECODE_AMDGPU_TARGET "AMD GPU target ASIC to build rocPyDecode" "")

# Set supported GPU Targets
set(DEFAULT_GPU_TARGETS "gfx908;gfx90a;gfx942;gfx1030;gfx1031;gfx1032;gfx1100;gfx1101;gfx1102;gfx1200;gfx1201")

# Set AMD GPU_TARGETS
if((AMDGPU_TARGETS OR DEFINED ENV{AMDGPU_TARGETS}) AND (NOT GPU_TARGETS))
    message("-- ${Red}${PROJECT_NAME} DEPRECATION -- AMDGPU_TARGETS use is deprecated. Use GPU_TARGETS${ColourReset}")
    if(DEFINED ENV{AMDGPU_TARGETS})
        set(GPU_TARGETS $ENV{AMDGPU_TARGETS} CACHE STRING "List of specific machine types for library to target")
    else()
        set(GPU_TARGETS ${AMDGPU_TARGETS})
    endif()
endif()

if(DEFINED ENV{GPU_ARCHS})
    set(GPU_TARGETS $ENV{GPU_ARCHS} CACHE STRING "List of specific machine types for library to target")
elseif(GPU_TARGETS)
    message("-- ${White}${PROJECT_NAME} -- GPU_TARGETS set with -D option${ColourReset}")
else()
    set(GPU_TARGETS "${DEFAULT_GPU_TARGETS}" CACHE STRING "List of specific machine types for library to target")
endif()
message("-- ${White}${PROJECT_NAME} -- AMD GPU_TARGETS: ${GPU_TARGETS}${ColourReset}")

find_package(HIP QUIET)
find_package(FFmpeg QUIET)
find_package(rocDecode QUIET)

find_package(Python3 QUIET COMPONENTS Interpreter Development)
message("-- ${Green}NOTE: rocPyDecode requires Python3, Found --  ${Python3_EXECUTABLE} ${ColourReset}")

find_package(pybind11 REQUIRED)
if(pybind11_FOUND)
    message("-- ${Green}NOTE: rocPyDecode requires pybind11, Found --  ${pybind11_INCLUDE_DIRS} ${ColourReset}")
endif()

find_package(dlpack REQUIRED)
if(dlpack_FOUND)
    message("-- ${Green}NOTE: rocPyDecode requires dlpack, Found")
endif()

if(HIP_FOUND AND rocDecode_FOUND AND pybind11_FOUND AND Python3_FOUND AND FFMPEG_FOUND)

    # avoid setting the default installation path to /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        message("-- ${Green}NOTE: rocPyDecode default installation path -- ${Python3_STDARCH} ${ColourReset}")
        set(CMAKE_INSTALL_PREFIX_PYTHON ${Python3_STDARCH} CACHE PATH "rocPyDecode PyBind default installation path" FORCE)
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # HIP
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} hip::device)
    # FFMPEG
    include_directories(${AVUTIL_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR})
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${FFMPEG_LIBRARIES})
    # rocDecode
    include_directories(${rocDecode_INCLUDE_DIRS}
                        ${ROCM_PATH}/share/rocdecode/utils
                        ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode)
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${rocDecode_LIBRARIES})
    # Python3
    include_directories(${Python3_INCLUDE_DIRS})
    # PyBind11
    include_directories(${pybind11_INCLUDE_DIRS})
    # rocPyDecode include
    include_directories(src)

    file(GLOB_RECURSE pyfiles pyRocVideoDecode/*.py pyRocVideoDecode/*.pyi)
    file(GLOB_RECURSE include src/*.h)
    file(GLOB sources src/*.cpp  ${ROCM_PATH}/share/rocdecode/utils/*.cpp ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode/*.cpp)

    pybind11_add_module(${TARGET_NAME} MODULE ${sources})

    target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARY_LIST})
    message("-- ${White}rocPyDecode -- Link Libraries: ${LINK_LIBRARY_LIST}${ColourReset}")
    message("-- ${White}rocPyDecode -- CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}${ColourReset}")

    # TODO: remove after ROCDECODE_CHECK_VERSION macro is pushed to mainline
    if(${ROCDECODE_MAJOR_VERSION} VERSION_GREATER_EQUAL "0" AND ${ROCDECODE_MINOR_VERSION} VERSION_GREATER_EQUAL "6" AND ${ROCDECODE_MICRO_VERSION} VERSION_GREATER_EQUAL "0")
        target_compile_definitions(${TARGET_NAME} PUBLIC OVERHEAD_SUPPORT=1)
    else()
        target_compile_definitions(${TARGET_NAME} PUBLIC OVERHEAD_SUPPORT=0)
    endif()

    # TODO: remove after ROCDECODE_CHECK_VERSION macro is pushed to mainline
    if(${ROCDECODE_MAJOR_VERSION} VERSION_GREATER_EQUAL "0" AND ${ROCDECODE_MINOR_VERSION} VERSION_GREATER_EQUAL "7" AND ${ROCDECODE_MICRO_VERSION} VERSION_GREATER_EQUAL "0")
        target_compile_definitions(${TARGET_NAME} PUBLIC CODEC_SUPPORTED_CHECK=1)
    else()
        target_compile_definitions(${TARGET_NAME} PUBLIC CODEC_SUPPORTED_CHECK=0)
    endif()

    # TODO: remove after ROCDECODE_CHECK_VERSION macro is pushed to mainline
    if(${ROCDECODE_MAJOR_VERSION} VERSION_GREATER_EQUAL "0" AND ${ROCDECODE_MINOR_VERSION} VERSION_GREATER_EQUAL "10" AND ${ROCDECODE_MICRO_VERSION} VERSION_GREATER_EQUAL "0")
        target_compile_definitions(${TARGET_NAME} PUBLIC MD5_MOVED_CHECK=1)
    else()
        target_compile_definitions(${TARGET_NAME} PUBLIC MD5_MOVED_CHECK=0)
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}")
    set(TARGET_PYMODULE_NAME "${PYTHON_MODULE_PREFIX}${TARGET_NAME}${PYTHON_MODULE_EXTENSION}")

    foreach (filename ${pyfiles})
        get_filename_component(target "${filename}" REALPATH)
        #to maintain folder structure
        file(RELATIVE_PATH ITEM_PATH_REL ${CMAKE_CURRENT_SOURCE_DIR} ${filename})
        message(STATUS "Copying ${filename} to ${TARGET_NAME}/${ITEM_PATH_REL}")
        configure_file("${filename}" "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${ITEM_PATH_REL}" COPYONLY)
    endforeach (filename)

    #install rocPyDecode pybind libs
    install(FILES ${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}/${TARGET_PYMODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX_PYTHON} COMPONENT runtime)

    message("-- ${Green}ROCm Video Decode Library Python Binding - rocPyDecode module added ${ColourReset}")
    message("-- ${BoldBlue}rocPyDecode Version -- ${VERSION}${ColourReset}")
    message("-- ${BoldBlue}rocPyDecode Install Path -- ${CMAKE_INSTALL_PREFIX_PYTHON}${ColourReset}")

    # make test with CTest
    enable_testing()
    include(CTest)
    add_subdirectory(samples)
else()
    message("-- ${Red}WARNING: rocPyDecode module excluded - Dependency Failure${ColourReset}")
    # Python3
    if(NOT Python3_FOUND)
        message("-- ${Yellow}NOTE: rocPyDecode pybind requires Python3, Not Found${ColourReset}")
    endif()
    # PyBind11
    if(NOT pybind11_FOUND)
        message("-- ${Yellow}NOTE: rocPyDecode requires pybind11, Not Found${ColourReset}")	
    endif()
    # rocDecode
    if(NOT rocDecode_FOUND)
        message("-- ${Yellow}NOTE: rocPyDecode requires rocDecode, Not Found${ColourReset}")	
    endif()
endif()