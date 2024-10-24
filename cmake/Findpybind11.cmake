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

string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" PYTHON_VERSION_SHORT "${Python3_VERSION}")

set(pybind11_LIBRARY "")
set(pybind11_DEFINITIONS USING_pybind11)
set(pybind11_VERSION_TYPE "")

find_path(pybind11_INCLUDE_DIR
        NAMES pybind11/pybind11.h
        HINTS
        /usr/local/lib/
        /usr/lib/
        /usr/bin/
        /usr/local/lib/python${PYTHON_VERSION_SHORT}/dist-packages/pybind11/
        /usr/local/lib/python${PYTHON_VERSION_SHORT}/dist-packages/pybind11/include/
        /usr/local/lib/python${PYTHON_VERSION_SHORT}/dist-packages/pybind11/share/cmake/
        ${Python3_EXECUTABLE}
        PATHS
        /usr/local/lib/python${PYTHON_VERSION_SHORT}/dist-packages/pybind11/
        /usr/local/lib/python${PYTHON_VERSION_SHORT}/dist-packages/pybind11/include/
        /usr/local/lib/python${PYTHON_VERSION_SHORT}/dist-packages/pybind11/share/cmake/
        ${Python3_EXECUTABLE}
)
set(pybind11_INCLUDE_DIRS ${pybind11_INCLUDE_DIR})
get_filename_component(PYBIND11_CMAKE_FOLDER "${pybind11_INCLUDE_DIRS}/../share/cmake/pybind11/" ABSOLUTE)
message(STATUS "pybind11: ${pybind11_INCLUDE_DIR} (found version \"${pybind11_VERSION}${pybind11_VERSION_TYPE}\") -- PYBIND11_CMAKE_FOLDER: ${PYBIND11_CMAKE_FOLDER}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pybind11
        FOUND_VAR pybind11_FOUND
        REQUIRED_VARS pybind11_INCLUDE_DIR
)
message(STATUS "pybind11 was FOUND: ${pybind11_FOUND} --- HEADERS: ${pybind11_INCLUDE_DIR}"  )
mark_as_advanced(pybind11_FOUND)

check_required_components(pybind11)
include("${PYBIND11_CMAKE_FOLDER}/pybind11Targets.cmake")
add_library(pybind11::headers IMPORTED INTERFACE)
set_target_properties(pybind11::headers PROPERTIES INTERFACE_LINK_LIBRARIES pybind11::pybind11_headers)
include("${PYBIND11_CMAKE_FOLDER}/pybind11Common.cmake")
