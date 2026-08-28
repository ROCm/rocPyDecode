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

cmake_minimum_required(VERSION 3.15)
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

# lib version - NOTE: update version across multiple files
set(VERSION "0.8.0")

# Set Project Version and Language
project(rocpydecode VERSION ${VERSION} LANGUAGES CXX)
set(TARGET_NAME rocpydecode)
set(TARGET_NAME_JPEG rocpyjpegdecode)

set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Library install directory")
set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE STRING "Include install directory")
include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

find_program(MAKE_NSIS_EXE makensis)
find_program(RPMBUILD_EXE rpmbuild)
find_program(DPKG_EXE dpkg)

# avoid setting the default installation path to /usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${ROCM_PATH} CACHE PATH "rocPyDecode default installation path" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

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
message("-- ${BoldBlue}rocPyDecode Version -- ${VERSION}${ColourReset}")
message("-- ${BoldBlue}rocPyDecode Install Path -- ${CMAKE_INSTALL_PREFIX}${ColourReset}")

# Check if lib/rocm_sysdeps/lib exists in the ROCm path which indicates ROCm installation via TheRock
set(USING_THE_ROCK OFF)
if(EXISTS "${ROCM_PATH}/lib/rocm_sysdeps/lib")
  set(USING_THE_ROCK ON)
endif()
if(USING_THE_ROCK)
  if(NOT DEFINED ENV{HIP_DEVICE_LIB_PATH})
    set(ENV{HIP_DEVICE_LIB_PATH} ${ROCM_PATH}/lib/llvm/amdgcn/bitcode)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "HIP_DEVICE_LIB_PATH=${ROCM_PATH}/lib/llvm/amdgcn/bitcode")
  endif()
endif()

# CMake Config location
list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH}/lib/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

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

option(ROCPYDECODE_ENABLE_WARNINGS "Enable compiler warnings for C++ targets" ON)
set(ROCPYDECODE_WARNING_FLAGS "")
if(ROCPYDECODE_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        list(APPEND ROCPYDECODE_WARNING_FLAGS -Wall)
    elseif(MSVC)
        list(APPEND ROCPYDECODE_WARNING_FLAGS
            /W4
            /permissive-)
    endif()

    if(ROCPYDECODE_WARNING_FLAGS)
        add_compile_options(${ROCPYDECODE_WARNING_FLAGS})
    endif()
endif()

# Set supported GPU Targets
if(NOT GPU_TARGETS AND NOT AMDGPU_TARGETS)
    find_package(ROCmCMakeBuildTools QUIET)
    if(NOT ROCmCMakeBuildTools_FOUND)
        find_package(ROCM QUIET)
    endif()
    include(ROCMCheckTargetIds OPTIONAL RESULT_VARIABLE HAS_ROCM_CHECK_TARGET_IDS)

    set(OPTIONAL_GPU_TARGETS "gfx1250")
    if(HAS_ROCM_CHECK_TARGET_IDS)
        rocm_check_target_ids(OPTIONAL_GPU_TARGETS_AVAILABLE TARGETS ${OPTIONAL_GPU_TARGETS})
    else() # if we don't have rocm_check_target_ids, just assume the targets are available
        set(OPTIONAL_GPU_TARGETS_AVAILABLE "${OPTIONAL_GPU_TARGETS}")
    endif()
    set(DEFAULT_GPU_TARGETS "gfx908;gfx90a;gfx942;gfx1030;gfx1031;gfx1032;gfx1100;gfx1101;gfx1102;gfx950;gfx1200;gfx1201;${OPTIONAL_GPU_TARGETS_AVAILABLE}")
endif()

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

# Allow specifying a Python version via -D PYTHON_VERSION_SUGGESTED
option(PYTHON_VERSION_SUGGESTED "Suggested Python version for installation" "")
set(USER_DEFINED FALSE)
if(NOT (PYTHON_VERSION_SUGGESTED STREQUAL OFF))
    set(USER_DEFINED TRUE)
    message("-- ${White}${USER_DEFINED} -- USER DEFINED PYTHON: ${PYTHON_VERSION_SUGGESTED}${ColourReset}")
endif()

# Python versions to probe (highest to lowest). User suggestion replaces the list.
set(PYTHON_VERSION_LIST 3.13 3.12 3.11 3.10 3.9 3.8 CACHE STRING "Python versions to probe")
if(USER_DEFINED)
    set(PYTHON_VERSION_LIST ${PYTHON_VERSION_SUGGESTED})
endif()

# rocPyDecode build
set(CMAKE_SKIP_BUILD_RPATH true)

# Force HIP to use the ROCm backend for this configure
set (HIP_PLATFORM amd CACHE STRING "HIP platform")

# Find dependencies
find_package(HIP REQUIRED)
find_package(rocdecode 1.0.0 QUIET)
find_package(rocjpeg 1.0.0 QUIET)
find_package(pybind11 REQUIRED)
find_package(DLPACK REQUIRED)
find_package(FFmpeg QUIET)
set(ROCPYDECODE_USE_FFMPEG FALSE)
set(ROCPYDECODE_FFMPEG_UTILS_DIR "${ROCM_PATH}/share/rocdecode/utils/ffmpegvideodecode")
if(FFMPEG_FOUND AND EXISTS "${ROCPYDECODE_FFMPEG_UTILS_DIR}/ffmpeg_video_dec.h")
    set(ROCPYDECODE_USE_FFMPEG TRUE)
elseif(FFMPEG_FOUND)
    message(STATUS "FFmpeg found, but rocdecode FFmpeg utility sources are unavailable; building without demux/CPU backend.")
endif()

set(ROCPYDECODE_DEPENDENCY_READY FALSE)
set(ROCPYJPEG_DEPENDENCY_READY FALSE)
set(ROCPYDECODE_HOST_LIBRARY_FOUND FALSE)

# HIP
set(LINK_LIBRARY_LIST hip::device)
set(LINK_LIBRARY_LIST_JPEG hip::device)
# rocDecode
set(pyfiles "")
if(rocdecode_FOUND)
    include_directories(src)
    file(GLOB pyfiles pyRocVideoDecode/*.py pyRocVideoDecode/*.pyi)

    # TBD (essam):
    # # The always-built rocdecode utility sources use libavutil's MD5 helpers.
    # if(AVUTIL_LIBRARY)
    #     list(APPEND LINK_LIBRARY_LIST ${AVUTIL_LIBRARY})
    # else()
    #     message(FATAL_ERROR "libavutil is required to build rocPyDecode because ROCm's roc_video_dec utilities use av_md5_* symbols.")
    # endif()

    # Always build the GPU path
    include_directories(SYSTEM
        ${rocdecode_INCLUDE_DIR}
        ${ROCM_PATH}/include/rocdecode
        ${ROCM_PATH}/share/rocdecode/utils
        ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode)
    if(rocdecode_VERSION VERSION_LESS_EQUAL 1.3.0)
        find_library(rocdecode_HOST_LIBRARY rocdecodehost PATHS ${ROCM_PATH}/lib)
    else()
        find_library(rocdecode_HOST_LIBRARY rocdecode-host PATHS ${ROCM_PATH}/lib)
    endif()

    list(APPEND LINK_LIBRARY_LIST rocdecode::rocdecode)
    if(rocdecode_HOST_LIBRARY)
        list(APPEND LINK_LIBRARY_LIST ${rocdecode_HOST_LIBRARY})
        set(ROCPYDECODE_HOST_LIBRARY_FOUND TRUE)
        message(STATUS "rocdecode_HOST_LIBRARY found")
    else()
        message(WARNING "Library rocdecode_HOST_LIBRARY not found! CPU backend support and samples will be limited.")
    endif()

    set(ROCPYDECODE_DEPENDENCY_READY TRUE)

    # Core GPU sources
    file(GLOB include_base src/rocdecode/*.h src/common/*.h ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode/*.h)
    file(GLOB sources_base src/rocdecode/roc_pydecode.cpp src/rocdecode/roc_pyvideodecode.cpp src/common/*.cpp ${ROCM_PATH}/share/rocdecode/utils/*.cpp ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode/*.cpp)
    file(GLOB sources_external_rocdecode ${ROCM_PATH}/share/rocdecode/utils/*.cpp ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode/*.cpp)
    set(include ${include_base})
    set(sources ${sources_base})
    if(sources_external_rocdecode)
        set_source_files_properties(${sources_external_rocdecode} PROPERTIES COMPILE_OPTIONS "-w")
    endif()

    if(ROCPYDECODE_USE_FFMPEG)
        # Demux + CPU backend (FFmpeg-dependent)
        file(GLOB sources_ffmpeg src/rocdecode/roc_pyvideodemuxer.cpp src/rocdecode/roc_pyvideodecodecpu.cpp src/rocdecode/roc_pydecode_test.cpp ${ROCM_PATH}/share/rocdecode/utils/ffmpegvideodecode/*.cpp)
        file(GLOB include_ffmpeg ${ROCM_PATH}/share/rocdecode/utils/ffmpegvideodecode/*.h)
        file(GLOB sources_external_ffmpeg ${ROCM_PATH}/share/rocdecode/utils/ffmpegvideodecode/*.cpp)
        list(APPEND include ${include_ffmpeg})
        list(APPEND sources ${sources_ffmpeg})
        include_directories(SYSTEM ${AVUTIL_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR})
        set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${FFMPEG_LIBRARIES})
        if(sources_external_ffmpeg)
            set_source_files_properties(${sources_external_ffmpeg} PROPERTIES COMPILE_OPTIONS "-w")
        endif()
    else()
        message(STATUS "FFmpeg demux/CPU backend unavailable; building rocPyDecode with GPU decode only.")
        set(ROCPYDECODE_HOST_LIBRARY_FOUND FALSE)
    endif()
else()
    message(WARNING "rocdecode not found. rocPyDecode will not be built or installed.")
endif()
# rocJPEG
if(rocjpeg_FOUND)
    include_directories(SYSTEM ${ROCM_PATH}/include ${rocjpeg_INCLUDE_DIR} ${ROCM_PATH}/share/rocjpeg/samples)
    set(LINK_LIBRARY_LIST_JPEG ${LINK_LIBRARY_LIST_JPEG} rocjpeg::rocjpeg)

    file(GLOB pyfiles_jpeg pyRocJpegDecode/*.py pyRocJpegDecode/*.pyi)
    file(GLOB include_jpeg src/rocjpeg/*.h src/common/*.h)
    file(GLOB sources_jpeg src/rocjpeg/*.cpp src/common/*.cpp)
    set(ROCPYJPEG_DEPENDENCY_READY TRUE)
else()
    message(WARNING "rocjpeg not found. rocPyJPEG will not be built or installed.")
endif()

if(ROCPYDECODE_DEPENDENCY_READY)
    message("-- ${White}rocPyDecode -- Link Libraries: ${LINK_LIBRARY_LIST}${ColourReset}")
else()
    message("-- ${Yellow}rocPyDecode skipped -- missing rocdecode dependency${ColourReset}")
endif()
if(ROCPYJPEG_DEPENDENCY_READY)
    message("-- ${White}rocPyJPEG -- Link Libraries: ${LINK_LIBRARY_LIST_JPEG}${ColourReset}")
else()
    message("-- ${Yellow}rocPyJPEG skipped -- missing rocjpeg dependency${ColourReset}")
endif()
message("-- ${White}rocPyDecode/rocPyJPEG -- CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}${ColourReset}")
message("-- ${White}rocPyDecode/rocPyJPEG -- Warning Flags:${ROCPYDECODE_WARNING_FLAGS}${ColourReset}")

# set license information
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
install(FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT runtime)
install(FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION ${CMAKE_INSTALL_DOCDIR}-asan COMPONENT asan)

#install rocPyDecode pybind lib
set(ROCPYDECODE_BUILT_TARGETS "")
set(ROCPYJPEG_INSTALLED FALSE)
if(ROCPYDECODE_DEPENDENCY_READY OR ROCPYJPEG_DEPENDENCY_READY)

    foreach(PYTHON_VERSION ${PYTHON_VERSION_LIST})
        message(STATUS "Searching for Python version: ${PYTHON_VERSION}")
        foreach(v IN ITEMS
            Python3_FOUND
            Python3_EXECUTABLE
            Python3_VERSION
            Python3_SOABI
            Python3_EXTENSION_SUFFIX
            Python3_Interpreter_FOUND
            Python3_Development_FOUND
            Python3_INCLUDE_DIRS
            Python3_LIBRARY
            Python3_LIBRARIES
            Python3_STDARCH
            Python3_FIND_IMPLEMENTATIONS)
            unset(${v} CACHE)
            unset(${v})
        endforeach()

        set(Python3_FIND_VIRTUALENV FIRST)
        find_package(Python3 ${PYTHON_VERSION} EXACT QUIET COMPONENTS Interpreter Development)

        if(NOT Python3_FOUND)
            message("-- ${Yellow}Python ${PYTHON_VERSION} not found; skipping${ColourReset}")
            continue()
        endif()

        string(REPLACE "." "_" PY_VERSION_SUFFIX "${PYTHON_VERSION}")

        if(ROCPYDECODE_DEPENDENCY_READY)
            set(TARGET_NAME_VERSIONED "${TARGET_NAME}_${PY_VERSION_SUFFIX}")
            pybind11_add_module(${TARGET_NAME_VERSIONED} MODULE ${sources})
            target_link_libraries(${TARGET_NAME_VERSIONED} PRIVATE ${LINK_LIBRARY_LIST} ${Python3_LIBRARIES})

            if(USING_THE_ROCK)
                set_target_properties(${TARGET_NAME_VERSIONED} PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/rocm_sysdeps/lib" BUILD_WITH_INSTALL_RPATH TRUE)
            endif()

            target_compile_definitions(${TARGET_NAME_VERSIONED} PRIVATE
                ROCPYDECODE_ENABLE_HOST=$<BOOL:${ROCPYDECODE_HOST_LIBRARY_FOUND}>
                ROCPYDECODE_USE_FFMPEG=$<BOOL:${ROCPYDECODE_USE_FFMPEG}>)

            target_include_directories(${TARGET_NAME_VERSIONED} PRIVATE ${pybind11_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)

            # FFMPEG multi-version support
            if(ROCPYDECODE_USE_FFMPEG)
                if(_FFMPEG_AVCODEC_VERSION VERSION_LESS_EQUAL 58.134.100)
                    target_compile_definitions(${TARGET_NAME_VERSIONED} PUBLIC USE_AVCODEC_GREATER_THAN_58_134=0)
                else()
                    target_compile_definitions(${TARGET_NAME_VERSIONED} PUBLIC USE_AVCODEC_GREATER_THAN_58_134=1)
                endif()
            endif()

            foreach(filename ${pyfiles})
                get_filename_component(target "${filename}" REALPATH)
                file(RELATIVE_PATH ITEM_PATH_REL ${CMAKE_CURRENT_SOURCE_DIR} ${filename})
                message(STATUS "Copying ${filename} to ${TARGET_NAME_VERSIONED}/${ITEM_PATH_REL}")
                configure_file("${filename}" "${CMAKE_BINARY_DIR}/${TARGET_NAME_VERSIONED}/${ITEM_PATH_REL}" COPYONLY)
            endforeach(filename)

            # Get the correct extension suffix directly from Python
            execute_process(
                COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"
                OUTPUT_VARIABLE Python3_EXTENSION_SUFFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            set_target_properties(${TARGET_NAME_VERSIONED} PROPERTIES
                PREFIX ""
                SUFFIX "${Python3_EXTENSION_SUFFIX}"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME_VERSIONED}/${CMAKE_INSTALL_LIBDIR}"
                OUTPUT_NAME "${TARGET_NAME}")

            list(APPEND ROCPYDECODE_BUILT_TARGETS ${TARGET_NAME_VERSIONED})
            message(STATUS "Building and installing rocPyDecode for ${PYTHON_VERSION}")
            install(TARGETS ${TARGET_NAME_VERSIONED} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime)
        endif()

        if(ROCPYJPEG_DEPENDENCY_READY)
            set(TARGET_NAME_JPEG_VERSIONED "${TARGET_NAME_JPEG}_${PY_VERSION_SUFFIX}")
            pybind11_add_module(${TARGET_NAME_JPEG_VERSIONED} MODULE ${sources_jpeg})
            target_link_libraries(${TARGET_NAME_JPEG_VERSIONED} PRIVATE ${LINK_LIBRARY_LIST_JPEG} ${Python3_LIBRARIES})
            if(USING_THE_ROCK)
                set_target_properties(${TARGET_NAME_JPEG_VERSIONED} PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/rocm_sysdeps/lib" BUILD_WITH_INSTALL_RPATH TRUE)
            endif()
            target_include_directories(${TARGET_NAME_JPEG_VERSIONED} PRIVATE ${pybind11_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
            foreach(filename ${pyfiles_jpeg})
                get_filename_component(target "${filename}" REALPATH)
                file(RELATIVE_PATH ITEM_PATH_REL ${CMAKE_CURRENT_SOURCE_DIR} ${filename})
                message(STATUS "Copying ${filename} to ${TARGET_NAME_JPEG_VERSIONED}/${ITEM_PATH_REL}")
                configure_file("${filename}" "${CMAKE_BINARY_DIR}/${TARGET_NAME_JPEG_VERSIONED}/${ITEM_PATH_REL}" COPYONLY)
            endforeach(filename)

            # Get the correct extension suffix directly from Python
            execute_process(
                COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"
                OUTPUT_VARIABLE Python3_EXTENSION_SUFFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            set_target_properties(${TARGET_NAME_JPEG_VERSIONED} PROPERTIES
                PREFIX ""
                SUFFIX "${Python3_EXTENSION_SUFFIX}"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME_JPEG_VERSIONED}/${CMAKE_INSTALL_LIBDIR}"
                OUTPUT_NAME "${TARGET_NAME_JPEG}")

            message(STATUS "Building and installing rocpyjpegdecode for ${PYTHON_VERSION}")
            install(TARGETS ${TARGET_NAME_JPEG_VERSIONED} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime)
            set(ROCPYJPEG_INSTALLED TRUE)
        endif()
    endforeach()
endif()

# Was rocPyDecode Installed?
if(NOT ROCPYDECODE_DEPENDENCY_READY)
    message("-- ${Yellow}rocPyDecode wasn't built or installed, missing rocdecode dependency.${ColourReset}")
elseif(ROCPYDECODE_BUILT_TARGETS STREQUAL "")
    if(USER_DEFINED)
        message("-- ${Red}rocPyDecode wasn't installed, user defined python version ${PYTHON_VERSION_SUGGESTED} was not found.${ColourReset}")
    else()
        message("-- ${Red}rocPyDecode wasn't installed, no python installation was found.${ColourReset}")
    endif()
endif()
if(NOT ROCPYJPEG_DEPENDENCY_READY)
    message("-- ${Yellow}rocPyJPEG wasn't built or installed, missing rocjpeg dependency.${ColourReset}")
elseif(NOT ROCPYJPEG_INSTALLED)
    if(USER_DEFINED)
        message("-- ${Red}rocPyJPEG wasn't installed, user defined python version ${PYTHON_VERSION_SUGGESTED} was not found.${ColourReset}")
    else()
        message("-- ${Red}rocPyJPEG wasn't installed, no python installation was found.${ColourReset}")
    endif()
endif()

# install rocPyDecode API folder -and- samples folder
if(ROCPYDECODE_DEPENDENCY_READY)
    install(DIRECTORY pyRocVideoDecode DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime)
    if(ROCPYDECODE_HOST_LIBRARY_FOUND)
        install(DIRECTORY samples/rocdecode DESTINATION ${CMAKE_INSTALL_LIBDIR}/pyRocVideoDecode/samples COMPONENT runtime)
    else()
        install(DIRECTORY samples/rocdecode DESTINATION ${CMAKE_INSTALL_LIBDIR}/pyRocVideoDecode/samples COMPONENT runtime
            PATTERN "videodecode_cpu_backend.py" EXCLUDE
            PATTERN "videodecodetorch_cpu_backend.py" EXCLUDE)
    endif()
endif()
# install rocpyjpegdecode API folder -and- samples folder
if(ROCPYJPEG_DEPENDENCY_READY)
    install(DIRECTORY pyRocJpegDecode DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime)
    install(DIRECTORY samples/rocjpeg DESTINATION ${CMAKE_INSTALL_LIBDIR}/pyRocJpegDecode/samples COMPONENT runtime)
endif()

# Test - Installed -rocpydecode
install(DIRECTORY ${CMAKE_SOURCE_DIR}/tests DESTINATION ${CMAKE_INSTALL_DATADIR}/rocpydecode COMPONENT test)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake DESTINATION ${CMAKE_INSTALL_DATADIR}/rocpydecode COMPONENT test)

if(NOT ROCPYDECODE_BUILT_TARGETS STREQUAL "")
    message("-- ${Green}ROCm Video Decode Library Python Binding - rocPyDecode module added ${ColourReset}")
    message("-- ${BoldBlue}rocPyDecode Version -- ${VERSION}${ColourReset}")
    message("-- ${BoldBlue}rocPyDecode Install Path -- ${CMAKE_INSTALL_PREFIX}${ColourReset}")
endif()

# add symlinks to videos in rocDecode shared folder (best-effort, no counters)
if(ROCPYDECODE_DEPENDENCY_READY)
    set(ROCDECODE_VIDEO_DIR "${ROCM_PATH}/share/rocdecode/video")
    if(IS_DIRECTORY "${ROCDECODE_VIDEO_DIR}")
        file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data/videos")
        file(GLOB ROCDECODE_VIDEO_FILES "${ROCDECODE_VIDEO_DIR}/*")
        foreach(VID_FILE IN LISTS ROCDECODE_VIDEO_FILES)
            get_filename_component(VID_NAME "${VID_FILE}" NAME)
            set(VID_LINK "${CMAKE_SOURCE_DIR}/data/videos/${VID_NAME}")
            if(NOT EXISTS "${VID_LINK}")
                execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${VID_FILE}" "${VID_LINK}")
            endif()
        endforeach()
        message("-- ${BoldBlue}Ensured rocdecode sample videos are linked into ${CMAKE_SOURCE_DIR}/data/videos${ColourReset}")
    endif()
endif()

# make test with CTest
enable_testing()
include(CTest)
if(ROCPYDECODE_DEPENDENCY_READY AND ROCPYJPEG_DEPENDENCY_READY)
    add_subdirectory(tests)
else()
    message("-- ${Yellow}Skipping tests; rocPyDecode and rocPyJPEG dependencies not fully available.${ColourReset}")
endif()

message(" ----------------")
if(ROCPYDECODE_DEPENDENCY_READY)
    message(" rocPyDecode is ready to build/install with base lib 'rocdecode'")
else()
    message(" rocPyDecode can not be built or installed due to the missing base lib 'rocdecode'")
endif()
if(ROCPYJPEG_DEPENDENCY_READY)
    message(" rocPyJpeg   is ready to build/install with base lib 'rocjpeg'")
else()
    message(" rocPyJpeg   can not be built or installed due to the missing base lib 'rocjpeg'")
endif()
message(" ----------------")

# set package information
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_LICENSE "MIT")
# TBD: Change to generic support email
set(CPACK_PACKAGE_CONTACT "rocPyDecode Support <mivisionx.support@amd.com>")
set(CPACK_PACKAGE_VENDOR "AMD ROCm")
set(CPACK_PACKAGE_GROUP "Development/Tools")
set(CPACK_PACKAGE_HOMEPAGE "https://github.com/ROCm/rocPyDecode")

if(ENABLE_ASAN_PACKAGING)
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "AMD rocPyDecode address sanitizer libraries")
    set(CPACK_RPM_PACKAGE_SUMMARY "AMD rocPyDecode address sanitizer libraries")
else()
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "AMD rocPyDecode is a python high performance video decode SDK for AMD GPUs")
    set(CPACK_RPM_PACKAGE_SUMMARY "AMD rocPyDecode is a python high performance video decode SDK for AMD GPUs")
endif()

if(DEFINED ENV{ROCM_LIBPATCH_VERSION})
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}.$ENV{ROCM_LIBPATCH_VERSION}")
endif()

# Set the dependent packages
set(ROCPYDECODE_DEBIAN_PACKAGE_LIST "rocdecode-dev, rocjpeg-dev, libdlpack-dev, python3-dev, python3-pip")
# TBD: libdlpack-devel not available
set(ROCPYDECODE_RPM_PACKAGE_LIST "rocdecode-devel, rocjpeg-devel, python3-devel, python3-pip")

# '%{?dist}' breaks manual builds on debian systems due to empty Provides
execute_process(
    COMMAND rpm --eval %{?dist}
    RESULT_VARIABLE PROC_RESULT
    OUTPUT_VARIABLE EVAL_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(PROC_RESULT EQUAL "0" AND NOT EVAL_RESULT STREQUAL "")
    string(APPEND CPACK_RPM_PACKAGE_RELEASE "%{?dist}")
endif()

# set package 
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${CPACK_PACKAGE_HOMEPAGE})
set(CPACK_RPM_PACKAGE_URL ${CPACK_PACKAGE_HOMEPAGE})
set(CPACK_RPM_PACKAGE_AUTOREQPROV "no")
# Debian package
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "rocm-core, ${ROCPYDECODE_DEBIAN_PACKAGE_LIST}")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_LICENSE "MIT")
# Debian package - specific variable for ASAN
set(CPACK_DEBIAN_ASAN_PACKAGE_NAME "${PROJECT_NAME}-asan")
set(CPACK_DEBIAN_ASAN_PACKAGE_DEPENDS "rocm-core-asan, ${ROCPYDECODE_DEBIAN_PACKAGE_LIST}")
# Debian package - Test
set(CPACK_DEBIAN_TEST_PACKAGE_NAME "${PROJECT_NAME}-test")
set(CPACK_DEBIAN_TEST_PACKAGE_DEPENDS "rocm-core, ${CPACK_DEBIAN_RUNTIME_PACKAGE_NAME}")
# RPM package
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_RUNTIME_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_RPM_RUNTIME_PACKAGE_REQUIRES "rocm-core, ${ROCPYDECODE_RPM_PACKAGE_LIST}")
set(CPACK_RPM_RUNTIME_PACKAGE_LICENSE "MIT")
# RPM package specific variable for ASAN
set(CPACK_RPM_ASAN_PACKAGE_NAME "${PROJECT_NAME}-asan")
set(CPACK_RPM_ASAN_PACKAGE_REQUIRES "rocm-core-asan, ${ROCPYDECODE_RPM_PACKAGE_LIST}")
# RPM package specific variable for Test
set(CPACK_RPM_TEST_PACKAGE_NAME "${PROJECT_NAME}-test")
set(CPACK_RPM_TEST_PACKAGE_REQUIRES "rocm-core, ${CPACK_RPM_RUNTIME_PACKAGE_NAME}")

if(NOT ROCM_DEP_ROCMCORE)
    string(REGEX REPLACE ",? ?rocm-core," "" CPACK_RPM_RUNTIME_PACKAGE_REQUIRES ${CPACK_RPM_RUNTIME_PACKAGE_REQUIRES})
    string(REGEX REPLACE ",? ?rocm-core-asan," "" CPACK_RPM_ASAN_PACKAGE_REQUIRES ${CPACK_RPM_ASAN_PACKAGE_REQUIRES})
    string(REGEX REPLACE ",? ?rocm-core," "" CPACK_RPM_TEST_PACKAGE_REQUIRES ${CPACK_RPM_TEST_PACKAGE_REQUIRES})
    string(REGEX REPLACE ",? ?rocm-core," "" CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS ${CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS})
    string(REGEX REPLACE ",? ?rocm-core-asan," "" CPACK_DEBIAN_ASAN_PACKAGE_DEPENDS ${CPACK_DEBIAN_ASAN_PACKAGE_DEPENDS})
    string(REGEX REPLACE ",? ?rocm-core," "" CPACK_DEBIAN_TEST_PACKAGE_DEPENDS ${CPACK_DEBIAN_TEST_PACKAGE_DEPENDS})
endif()

if(ENABLE_ASAN_PACKAGING)
    # ASAN Package requires asan component with only libraries and license file
    set(CPACK_COMPONENTS_ALL asan)
else()
    set(CPACK_COMPONENTS_ALL runtime test)
endif()

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_RELEASE "local")
set(CPACK_RPM_PACKAGE_RELEASE "local")

if(DEFINED ENV{CPACK_DEBIAN_PACKAGE_RELEASE})
    set(CPACK_DEBIAN_PACKAGE_RELEASE $ENV{CPACK_DEBIAN_PACKAGE_RELEASE})
endif()
if(DEFINED ENV{CPACK_RPM_PACKAGE_RELEASE})
    set(CPACK_RPM_PACKAGE_RELEASE $ENV{CPACK_RPM_PACKAGE_RELEASE})
endif()

set(CPACK_GENERATOR "TGZ;ZIP")
if(EXISTS ${MAKE_NSIS_EXE})
    list(APPEND CPACK_GENERATOR "NSIS")
endif()
if(EXISTS ${RPMBUILD_EXE})
    list(APPEND CPACK_GENERATOR "RPM")
    message("-- ${White}rocPyDecode .rpm Package Requirements -- ${CPACK_RPM_RUNTIME_PACKAGE_REQUIRES}${ColourReset}")
    message("-- ${White}rocPyDecode .rpm Test Package Requirements -- ${CPACK_RPM_TEST_PACKAGE_REQUIRES}${ColourReset}")
endif()
if(EXISTS ${DPKG_EXE})
    list(APPEND CPACK_GENERATOR "DEB")
    message("-- ${White}rocPyDecode .deb Package Requirements -- ${CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS}${ColourReset}")
    message("-- ${White}rocPyDecode .deb Test Package Requirements -- ${CPACK_DEBIAN_TEST_PACKAGE_DEPENDS}${ColourReset}")
endif()

include(CPack)

cpack_add_component(runtime
    DISPLAY_NAME "rocPyDecode Package"
    DESCRIPTION "AMD rocPyDecode is a python high performance video decode SDK for AMD GPUs")

cpack_add_component(asan
    DISPLAY_NAME "rocPyDecode ASAN Package"
    DESCRIPTION "AMD rocPyDecode is a python high performance video decode SDK for AMD GPUs")

cpack_add_component(test
    DISPLAY_NAME "rocPyDecode Test Package"
    DESCRIPTION "AMD rocPyDecode is a python high performance video decode SDK for AMD GPUs")
