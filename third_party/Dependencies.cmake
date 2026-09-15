# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

# Both components consume the same sources; initialize shared targets only once.
get_property(_rocpydecode_deps_ready GLOBAL PROPERTY ROCPYDECODE_BUNDLED_DEPS_READY)
if(_rocpydecode_deps_ready)
    return()
endif()
if(TARGET pybind11::headers OR TARGET pybind11::module)
    message(FATAL_ERROR
        "rocPyDecode requires its bundled pybind11 3.1.0 targets. "
        "Add rocPyDecode before other projects that define pybind11 targets.")
endif()
if(TARGET dlpack OR TARGET dlpack::dlpack)
    message(FATAL_ERROR
        "rocPyDecode requires its bundled DLPack 1.3 target. "
        "Add rocPyDecode before other projects that define DLPack targets.")
endif()
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/third_party" EXCLUDE_FROM_ALL)
set_property(GLOBAL PROPERTY ROCPYDECODE_BUNDLED_DEPS_READY TRUE)
