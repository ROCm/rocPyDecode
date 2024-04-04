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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required (VERSION 3.5)
project(rocPyDecode)
set(TARGET_NAME rocPyDecode)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)

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
 
find_package(pybind11 REQUIRED)

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

include(FetchContent)
FetchContent_Populate(
  dlpack
  URL        https://github.com/dmlc/dlpack/archive/refs/tags/v0.8.zip
  SOURCE_DIR dlpack
)
message(STATUS " downloading dlpack library " "${dlpack_SOURCE_DIR}")
# fetchcontent_makeavailable(dlpack)

if(HIP_FOUND AND ROCDECODE_FOUND AND pybind11_FOUND AND Python3_FOUND AND FFMPEG_FOUND) 

    # avoid setting the default installation path to /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	
        message("-- ${Green}NOTE: rocPyDecode PyBind default installation path -- ${Python3_STDARCH} ${ColourReset}")	
        set(CMAKE_INSTALL_PREFIX_PYTHON ${Python3_STDARCH} CACHE PATH "rocPyDecode PyBind default installation path" FORCE)	
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	
    # HIP
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} hip::host)
    # FFMPEG
    include_directories(${AVUTIL_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR})
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${FFMPEG_LIBRARIES})
    # rocDecode
    include_directories (${ROCDECODE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${ROCDECODE_LIBRARY})

    # rocPyDecode include
    include_directories (${rocPyDecode_INCLUDE_DIR} /src)
    set(LINK_LIBRARY_LIST ${LINK_LIBRARY_LIST} ${rocPyDecode_LIBRARY})
    include_directories (${ROCM_PATH}/share/rocdecode/utils ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode "${dlpack_SOURCE_DIR}/include")
     
    file(GLOB_RECURSE pyfiles pyRocVideoDecode/*.py pyRocVideoDecode/*.pyi)
    file(GLOB_RECURSE include src/*.h)
    file(GLOB_RECURSE sources src/*.cpp  ${ROCM_PATH}/share/rocdecode/utils/rocvideodecode/*.cpp)

    pybind11_add_module(${TARGET_NAME} MODULE ${sources})
    
    target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARY_LIST})
    message("-- ${White}rocPyDecode -- Link Libraries: ${LINK_LIBRARY_LIST}${ColourReset}")
    message("-- ${White}rocPyDecode -- CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}${ColourReset}")
 
    set_target_properties(${TARGET_NAME} PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}")
    set(TARGET_PYMODULE_NAME "${PYTHON_MODULE_PREFIX}${TARGET_NAME}${PYTHON_MODULE_EXTENSION}")
                  
    CONFIGURE_FILE(setup.py
                ${CMAKE_BINARY_DIR}/setup.py @ONLY
                )
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
       
    #install rocPyDecode pybind libs
    install(FILES ${CMAKE_BINARY_DIR}/${TARGET_NAME}/${CMAKE_INSTALL_LIBDIR}/${TARGET_PYMODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX_PYTHON})

    message("-- ${Green}ROCm Video Decode Library Python Binding - rocPyDecode module added ${ColourReset}")
else()
    message("-- ${Red}WARNING: rocPyDecode module excluded - Dependency Failure${ColourReset}")
endif()
