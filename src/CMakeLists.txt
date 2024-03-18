set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required (VERSION 3.5)
project(rocPyDecode VERSION 1.0.0)

set(Python3_FIND_VIRTUALENV FIRST)	
find_package(Python3 QUIET COMPONENTS Interpreter Development)

option(FETCHCONTENT_QUIET OFF)
include(FetchContent)
fetchcontent_declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/v2.10.0.tar.gz
    URL_HASH
        SHA256=eacf582fa8f696227988d08cfc46121770823839fe9e301a20fbce67e7cd70ec
)
fetchcontent_makeavailable(pybind11)