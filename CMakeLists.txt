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

cmake_minimum_required (VERSION 3.5)
project(rocpydecode)
set(TARGET_NAME rocpydecode)
set(CMAKE_CXX_STANDARD 17)

# ROCM Path
if(DEFINED ENV{ROCM_PATH})
  set(ROCM_PATH $ENV{ROCM_PATH} CACHE PATH "${White}${PROJECT_NAME}: Default ROCm installation path${ColourReset}")
elseif(ROCM_PATH)
  message("-- ${White}${PROJECT_NAME} :ROCM_PATH Set -- ${ROCM_PATH}${ColourReset}")
else()
  set(ROCM_PATH /opt/rocm CACHE PATH "${White}${PROJECT_NAME}: Default ROCm installation path${ColourReset}")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH}/hip ${ROCM_PATH})
set(CMAKE_CXX_COMPILER ${ROCM_PATH}/llvm/bin/clang++)

# rocDecode sample build type
set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "rocDecode Default Build Type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  # -O0 -- Don't Optimize output file 
  # -gdwarf-4  -- generate debugging information, dwarf-4 for making valgrind work
  # -Og -- Optimize for debugging experience rather than speed or size
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -gdwarf-4 -Og")
else()
  # -O3       -- Optimize output file 
  # -DNDEBUG  -- turn off asserts 
  # -fPIC     -- Generate position-independent code if possible
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -fPIC")
endif()

set(Python3_FIND_VIRTUALENV FIRST)	
find_package(Python3 QUIET COMPONENTS Interpreter Development)

find_package(pybind11 QUIET CONFIG)
find_package(HIP QUIET)
find_package(FFmpeg QUIET)

if (PYTHONVERSION)
    find_package(Python3 "${PYTHONVERSION}" EXACT QUIET COMPONENTS Interpreter Development)
else()
    set(Python3_FIND_VIRTUALENV FIRST)
    find_package(Python3 QUIET COMPONENTS Interpreter Development)
endif()

# find rocDecode
find_library(ROCDECODE_LIBRARY NAMES rocdecode HINTS ${ROCM_PATH}/lib)
find_path(ROCDECODE_INCLUDE_DIR NAMES rocdecode.h PATHS /opt/rocm/include/rocdecode ${ROCM_PATH}/include/rocdecode)

if(ROCDECODE_LIBRARY AND ROCDECODE_INCLUDE_DIR)
    set(ROCDECODE_FOUND TRUE)
    message("-- ${Green}${PROJECT_NAME}: Using rocDecode -- \n\tLibraries:${ROCDECODE_LIBRARY} \n\tIncludes:${ROCDECODE_INCLUDE_DIR}${ColourReset}")
endif()

if(HIP_FOUND AND ROCDECODE_FOUND AND pybind11_FOUND AND Python3_FOUND AND FFMPEG_FOUND) 

    # avoid setting the default installation path to /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	
        message("-- ${Green}NOTE: rocpydecode PyBind default installation path -- ${Python3_STDARCH} ${ColourReset}")	
        set(CMAKE_INSTALL_PREFIX_PYTHON ${Python3_STDARCH} CACHE PATH "rocpydecode PyBind default installation path" FORCE)	
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	
    # HIP
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} hip::host)
    # FFMPEG
    include_directories(${AVUTIL_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR}
                        ${AVFORMAT_INCLUDE_DIR})
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${FFMPEG_LIBRARIES})
    # rocDecode
    include_directories (${ROCDECODE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${ROCDECODE_LIBRARY})

    # rocpydecode include
    include_directories (${rocpydecode_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/inc)
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${rocpydecode_LIBRARY})

    file(GLOB_RECURSE pyfiles amd/*.py amd/rocpydecode/*.py)
    file(GLOB_RECURSE include inc/*.cpp)
    file(GLOB_RECURSE sources src/*.cpp)

    pybind11_add_module(rocpydecode MODULE ${sources})
    
    target_link_libraries(rocpydecode PRIVATE ${LINK_LIBRARY_LIST})
    message("-- ${White}rocpydecode -- Link Libraries: ${LINK_LIBRARY_LIST}${ColourReset}")
    message("-- ${White}rocpydecode -- CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}${ColourReset}")
 
    set_target_properties(rocpydecode PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}")
    set(TARGET_PYMODULE_NAME "${PYTHON_MODULE_PREFIX}rocpydecode${PYTHON_MODULE_EXTENSION}")

    CONFIGURE_FILE(amd/__init__.py
                ${CMAKE_BINARY_DIR}/${TARGET_NAME}/__init__.py @ONLY
                )
    CONFIGURE_FILE(amd/rocpydecode/__init__.py
                 ${CMAKE_BINARY_DIR}/${TARGET_NAME}/__init__.py @ONLY
                 )                    
    CONFIGURE_FILE(setup.py
                ${CMAKE_BINARY_DIR}/setup.py @ONLY
                )
    # CONFIGURE_FILE(getrocdecodewheelname.py
    #             ${CMAKE_BINARY_DIR}/get_rocdecode_wheelname.py @ONLY
    #             )
    CONFIGURE_FILE(src/roc_pydecode.cpp
                ${CMAKE_BINARY_DIR}/roc_pydecode.cpp @ONLY
                )
    CONFIGURE_FILE(src/roc_pyvideodecode.cpp
                ${CMAKE_BINARY_DIR}/roc_pyvideodecode.cpp @ONLY
                )
    CONFIGURE_FILE(src/roc_pyvideodemuxer.cpp
                ${CMAKE_BINARY_DIR}/roc_pyvideodemuxer.cpp @ONLY
                )

    foreach (filename ${pyfiles})
        get_filename_component(target "${filename}" REALPATH)
        #to maintain folder structure
        file(RELATIVE_PATH ITEM_PATH_REL ${CMAKE_CURRENT_SOURCE_DIR} ${filename})
        message(STATUS "Copying ${filename} to ${TARGET_NAME}/${ITEM_PATH_REL}")
        configure_file("${filename}" "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${ITEM_PATH_REL}" COPYONLY)
    endforeach (filename)

    # execute_process(COMMAND "${Python3_EXECUTABLE}" get_rocdecode_wheelname.py OUTPUT_VARIABLE PY_WHEELFILE
    #     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    # message(STATUS "Detected rocpydecode Python wheel filename ${PY_WHEELFILE}")
        
    # add_custom_target(PyPackageBuild
    #     COMMAND "${Python3_EXECUTABLE}" -m pip wheel .
    #     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    #     COMMENT "Building python wheel package"
    #     )

    # add_dependencies(PyPackageBuild rocpydecode)

    # set_target_properties(PyPackageBuild PROPERTIES Package_Name ${PY_WHEELFILE})
    # add_custom_target(PyPackageInstall
    #         COMMAND "${Python3_EXECUTABLE}" -m pip install --upgrade --force-reinstall --no-deps "${PY_WHEELFILE}"
    #         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    #         COMMENT "Installing wheel file"
    #         )
    #add_dependencies(PyPackageInstall PyPackageBuild)
        
    #install rocpydecode pybind libs
    install(FILES ${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}/${TARGET_PYMODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX_PYTHON})
    message("-- ${Green}Radeon Decode Library Python Binding - rocpydecode module added ${ColourReset}")
else()
    message("-- ${Red}WARNING: Decode PyBind Module excluded - Dependency Failure${ColourReset}")
endif()
