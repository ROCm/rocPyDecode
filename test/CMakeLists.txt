################################################################################
#
# MIT License
#
# Copyright (c) 2023 Advanced Micro Devices, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
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
if (NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_C_COMPILER ${ROCM_PATH}/bin/amdclang)
    set(CMAKE_CXX_COMPILER ${ROCM_PATH}/bin/amdclang++)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

project(rocpydecode-pybind-test)

# make test with CTest
enable_testing()
include(CTest)

set(Python3_FIND_VIRTUALENV FIRST)
find_package(Python3 QUIET)

set(ROCPYDECODE_PYBIND_SCRIPTS OFF)
if(EXISTS "${ROCM_PATH}/lib/pyRocVideoDecode")
  set(ROCPYDECODE_PYBIND_SCRIPTS ON)
endif()

# Tests
if(Python3_FOUND AND ROCPYDECODE_PYBIND_SCRIPTS)
  message("-- ${PROJECT_NAME}: rocPyDecode found at ${ROCM_PATH}/lib/pyRocVideoDecode")
  # 1 - decoders test
  add_test(NAME rocpydecode_test_decoders
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decoders_test.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST rocpydecode_test_decoders PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
  # 2 - decodercpu test
  add_test(NAME rocpydecode_test_decodercpu
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decodercpu_test.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST rocpydecode_test_decodercpu PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
  # 3 - demuxer test
  add_test(NAME rocpydecode_test_demuxer
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/demuxer_test.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST rocpydecode_test_demuxer PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
  # 4 - types test
  add_test(NAME rocpydecode_test_types
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/types_test.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_property(TEST rocpydecode_test_types PROPERTY ENVIRONMENT "PYTHONPATH=${ROCM_PATH}/lib:$PYTHONPATH")
else()
  if(NOT Python3_FOUND)
    message("-- NOTE: rocPyDecode requires Python3 - NOT FOUND")
  endif()
  if(NOT ROCPYDECODE_PYBIND_SCRIPTS)
    message("-- NOTE: rocPyDecode requires the project to be built and installed - NOT FOUND")
  endif()
endif(Python3_FOUND AND ROCPYDECODE_PYBIND_SCRIPTS)
