# ##############################################################################
# Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc.
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
# ##############################################################################
cmake_minimum_required(VERSION 3.12)

set(Python3_FIND_VIRTUALENV FIRST)
find_package(Python3 QUIET)

# ROCM Path
if(DEFINED ENV{ROCM_PATH})
  set(ROCM_PATH $ENV{ROCM_PATH} CACHE PATH "${White}${PROJECT_NAME}: Default ROCm installation path${ColourReset}")
elseif(ROCM_PATH)
  message("-- ${White}${PROJECT_NAME} :ROCM_PATH Set -- ${ROCM_PATH}${ColourReset}")
else()
  set(ROCM_PATH /opt/rocm CACHE PATH "${White}${PROJECT_NAME}: Default ROCm installation path${ColourReset}")
endif()

# rocPyDecode tests
if(Python3_FOUND)
    # add python-hip for perf tests
    execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install --upgrade pip OUTPUT_VARIABLE PIP_VERSION ERROR_VARIABLE PIP_ERROR RESULT_VARIABLE PIP_RESULT)
    execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install -i https://test.pypi.org/simple hip-python OUTPUT_VARIABLE PIP_VERSION ERROR_VARIABLE PIP_ERROR RESULT_VARIABLE PIP_RESULT)

    #run tests
    add_test(NAME video_decode_python_H265
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/samples/videodecode.py
            -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME video_decode_perf_python_H265
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/samples/videodecodeperf.py
            -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME video_decode_rgb_python_H265
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/samples/videodecodergb.py
            -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
            -of 3
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME video_decode_python_H264
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/samples/videodecode.py
            -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME video_decode_perf_python_H264
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/samples/videodecodeperf.py
            -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    if(${ROCDECODE_MAJOR_VERSION} VERSION_GREATER_EQUAL "0" AND ${ROCDECODE_MINOR_VERSION} VERSION_GREATER_EQUAL "7" AND ${ROCDECODE_MICRO_VERSION} VERSION_GREATER_EQUAL "0")
        add_test(NAME video_decode_python_AV1
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/samples/videodecode.py
        -i ${ROCM_PATH}/share/rocdecode/video/AMD_driving_virtual_20-AV1.mp4
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}) 
    endif() 
endif(Python3_FOUND)
