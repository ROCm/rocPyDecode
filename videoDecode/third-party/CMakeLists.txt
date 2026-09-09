# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

# Only consume the upstream build helpers and headers.
set(PYBIND11_TEST OFF)
set(PYBIND11_INSTALL OFF)
set(PYBIND11_NOPYTHON OFF)
add_subdirectory(pybind11 EXCLUDE_FROM_ALL)
add_subdirectory(dlpack EXCLUDE_FROM_ALL)
