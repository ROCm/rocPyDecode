# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

# Only consume the upstream build helpers and headers.
set(PYBIND11_TEST OFF)
set(PYBIND11_INSTALL OFF)
set(PYBIND11_NOPYTHON OFF)
add_subdirectory(pybind11 EXCLUDE_FROM_ALL)
# DLPack is consumed as headers, matching rocCV's vendored layout.
add_library(dlpack INTERFACE)
target_include_directories(dlpack INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dlpack/include>)
add_library(dlpack::dlpack ALIAS dlpack)
