# ################################################################################
# # Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc.
# #
# # Permission is hereby granted, free of charge, to any person obtaining a copy
# # of this software and associated documentation files (the "Software"), to deal
# # in the Software without restriction, including without limitation the rights
# # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# # copies of the Software, and to permit persons to whom the Software is
# # furnished to do so, subject to the following conditions:
# #
# # The above copyright notice and this permission notice shall be included in all
# # copies or substantial portions of the Software.
# #
# # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# # SOFTWARE.
# #
# ################################################################################

execute_process(
    COMMAND sudo find / -name "pybind11Config.cmake" -not -path "*docker*"
    OUTPUT_VARIABLE PYBIND11_CMAKE_FILE_PATH 
    ERROR_VARIABLE ERROR_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND sudo dirname ${PYBIND11_CMAKE_FILE_PATH}
    OUTPUT_VARIABLE PYBIND11_CMAKE_FOLDER
    ERROR_VARIABLE ERROR_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

get_filename_component(PACKAGE_PREFIX_DIR "${PYBIND11_CMAKE_FOLDER}/../../../" ABSOLUTE)
 
# Location of pybind11/pybind11.h this will be relative
set(pybind11_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include")

set(pybind11_LIBRARY "")
set(pybind11_DEFINITIONS USING_pybind11)
set(pybind11_VERSION_TYPE "")

check_required_components(pybind11)

include("${PYBIND11_CMAKE_FOLDER}/pybind11Targets.cmake")

# Easier to use / remember
add_library(pybind11::headers IMPORTED INTERFACE)
set_target_properties(pybind11::headers PROPERTIES INTERFACE_LINK_LIBRARIES pybind11::pybind11_headers)

include("${PYBIND11_CMAKE_FOLDER}/pybind11Common.cmake")

set(pybind11_FOUND TRUE)

if(NOT pybind11_FIND_QUIETLY)
  message(
    STATUS
      "Found pybind11: ${pybind11_INCLUDE_DIR} (found version \"${pybind11_VERSION}${pybind11_VERSION_TYPE}\")"
  )
endif()
