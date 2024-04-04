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
project(rocPyDecode VERSION 1.0.0)

set(Python3_FIND_VIRTUALENV FIRST)	
find_package(Python3 QUIET COMPONENTS Interpreter Development)

option(FETCHCONTENT_QUIET ON)

include(FetchContent)
fetchcontent_declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/v2.10.0.tar.gz
    URL_HASH
        SHA256=eacf582fa8f696227988d08cfc46121770823839fe9e301a20fbce67e7cd70ec
)
fetchcontent_makeavailable(pybind11)

 