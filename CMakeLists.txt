################################################################################
# Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc.
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

cmake_minimum_required (VERSION 3.5)
project(rocPyDecode)
set(TARGET_NAME rocPyDecode)
option(BUILD_PYPACKAGE  "Build rocPyDecode Python Package" ON)

set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Library install directory")
include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)
set(ROCM_PATH /opt/rocm CACHE PATH "Default ROCm Installation Path")
set(CMAKE_CXX_STANDARD 17)
set(BUILD_ROCDECODE_PYBIND true)
set(ROCDECODE_PYTHON ON)
set(Python3_FIND_VIRTUALENV FIRST)	
find_package(Python3 QUIET COMPONENTS Interpreter Development)
find_package(pybind11 QUIET CONFIG)	
find_package(HALF QUIET)
find_package(FFmpeg QUIET)

# find rocDecode
find_library(ROCDECODE_LIBRARY NAMES rocdecode HINTS ${ROCM_PATH}/lib)
find_path(ROCDECODE_INCLUDE_DIR NAMES rocdecode.h PATHS /opt/rocm/include/rocdecode ${ROCM_PATH}/include/rocdecode)
find_path(ROCDECODE_INCLUDE_DIR NAMES rocparser.h PATHS /opt/rocm/include/rocparser ${ROCM_PATH}/include/rocparser)

# Backend
if(BUILD_PYPACKAGE)
    if(NOT DEFINED BACKEND)
        set(BACKEND "HIP")   # set default backend to HIP
    else()
        message("-- ${Red}Warning: rocPyDecode PyBind backend option: ${BACKEND} -- unknown${ColourReset}")
        message("-- ${Red}Warning: rocPyDecode PyBind default backend will be enforced${ColourReset}")
        set(BACKEND "HIP")
    endif()
    message("-- ${BoldBlue}rocPyDecode PyBind Backend set to -- ${BACKEND}${ColourReset}")

    # Find HIP Support
    if(GPU_SUPPORT AND "${BACKEND}" STREQUAL "HIP")
        if(NOT DEFINED HIP_PATH)
            if(NOT DEFINED ENV{HIP_PATH})
                set(HIP_PATH ${ROCM_PATH} CACHE PATH "Path to which HIP has been installed")
            else()
                set(HIP_PATH $ENV{HIP_PATH} CACHE PATH "Path to which HIP has been installed")
            endif()
        endif()
        list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH}/hip ${ROCM_PATH})
        find_package(HIP QUIET)
        if(HIP_FOUND)
            message("-- ${White}rocPyDecode PyBind -- Using HIP -- Path:" ${HIP_PATH} "\tVersion:" ${HIP_VERSION} "\tCompiler:" ${HIP_COMPILER} ${ColourReset})
        else()
            set(BUILD_ROCDECODE_PYBIND false)
            message("-- ${Yellow}rocPyDecode PyBind requires HIP, Not Found${ColourReset}")
        endif()
    endif()

endif()

# Dependency Check

if(${BUILD_ROCDECODE_PYBIND})
    # avoid setting the default installation path to /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	
        message("-- ${Green}NOTE: rocPyDecode PyBind default installation path -- ${Python3_STDARCH} ${ColourReset}")	
        set(CMAKE_INSTALL_PREFIX_PYTHON ${Python3_STDARCH} CACHE PATH "rocPyDecode PyBind default installation path" FORCE)	
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    
    # rocDecode
    include_directories (${ROCDECODE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)    
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${ROCDECODE_LIBRARY})
    include_directories(../../rocDecode/api)

    # FFMPEG
    include_directories(${AVUTIL_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR}
                        ${AVFORMAT_INCLUDE_DIR})
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${FFMPEG_LIBRARIES})
    # Python3	
    include_directories(${Python3_INCLUDE_DIRS})	
    message("-- ${Green}NOTE: rocPyDecode pybind requires Python3, Found --  ${Python3_EXECUTABLE} ${ColourReset}")	
    # PyBind11	
    include_directories(${pybind11_INCLUDE_DIRS})
    # half
    include_directories(${HALF_INCLUDE_DIRS})

    file(GLOB_RECURSE pyfiles amd/*.py)
    file(GLOB_RECURSE sources source/*.cpp)

    pybind11_add_module(rocPyDecode MODULE ${sources})
    target_link_libraries(rocPyDecode PRIVATE ${LINK_LIBRARY_LIST})
    message("-- ${White}rocPyDecode -- Link Libraries: ${LINK_LIBRARY_LIST}${ColourReset}")
    message("-- ${White}rocPyDecode -- CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}${ColourReset}")

    #temporary fix (for not finding rocdecode):
    add_definitions(-D __HIP_PLATFORM_AMD__)
    include_directories(/opt/rocm/include)

    set_target_properties(rocPyDecode PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}")
    set(TARGET_PYMODULE_NAME "${PYTHON_MODULE_PREFIX}rocPyDecode${PYTHON_MODULE_EXTENSION}")

    CONFIGURE_FILE(amd/__init__.py
                ${CMAKE_BINARY_DIR}/${TARGET_NAME}/__init__.py @ONLY
                )
    CONFIGURE_FILE(setup.py
                ${CMAKE_BINARY_DIR}/setup.py @ONLY
                )
    CONFIGURE_FILE(get_rocdecode_wheelname.py
                ${CMAKE_BINARY_DIR}/get_rocdecode_wheelname.py @ONLY
                )
    CONFIGURE_FILE(source/roc_pydecode.cpp
                ${CMAKE_BINARY_DIR}/roc_pydecode.cpp @ONLY
                )
    CONFIGURE_FILE(source/roc_pyvideodecode.cpp
                ${CMAKE_BINARY_DIR}/roc_pyvideodecode.cpp @ONLY
                )
    CONFIGURE_FILE(source/roc_pyvideodemuxer.cpp
                ${CMAKE_BINARY_DIR}/roc_pyvideodemuxer.cpp @ONLY
                )

    foreach (filename ${pyfiles})
        get_filename_component(target "${filename}" REALPATH)
        #to maintain folder structure
        file(RELATIVE_PATH ITEM_PATH_REL ${CMAKE_CURRENT_SOURCE_DIR} ${filename})
        message(STATUS "Copying ${filename} to ${TARGET_NAME}/${ITEM_PATH_REL}")
        configure_file("${filename}" "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${ITEM_PATH_REL}" COPYONLY)
    endforeach (filename)

    execute_process(COMMAND "${Python3_EXECUTABLE}" get_rocdecode_wheelname.py OUTPUT_VARIABLE PY_WHEELFILE
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    message(STATUS "Detected rocPyDecode Python wheel filename ${PY_WHEELFILE}")
        
    add_custom_target(PyPackageBuild
        COMMAND "${Python3_EXECUTABLE}" -m pip wheel .
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building python wheel package"
        )

    add_dependencies(PyPackageBuild rocPyDecode)

    set_target_properties(PyPackageBuild PROPERTIES Package_Name ${PY_WHEELFILE})
    add_custom_target(PyPackageInstall
            COMMAND "${Python3_EXECUTABLE}" -m pip install --upgrade --force-reinstall --no-deps "${PY_WHEELFILE}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Installing wheel file"
            )
    add_dependencies(PyPackageInstall PyPackageBuild)
        
    #install rocPyDecode pybind libs
    install(FILES ${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}/${TARGET_PYMODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX_PYTHON})
    message("-- ${Green}Radeon Decode Library Python Binding - rocPyDecode module added ${ColourReset}")
else()
    message("-- ${Red}WARNING: Decode PyBind Module excluded - Dependency Failure${ColourReset}")
endif()
