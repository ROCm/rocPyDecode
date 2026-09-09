# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

# Shared by the combined build and either component configured directly.
include_guard(GLOBAL)
if(TARGET pybind11::headers OR TARGET pybind11::module)
    message(FATAL_ERROR
        "rocPyDecode requires its bundled pybind11 2.11.1 targets. "
        "Add rocPyDecode before other projects that define pybind11 targets.")
endif()
if(TARGET dlpack OR TARGET dlpack::dlpack)
    message(FATAL_ERROR
        "rocPyDecode requires its bundled DLPack 1.3 target. "
        "Add rocPyDecode before other projects that define DLPack targets.")
endif()
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/third-party" EXCLUDE_FROM_ALL)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/pybind11/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/rocpydecode/third-party/pybind11")
install(FILES "${CMAKE_CURRENT_LIST_DIR}/dlpack/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/rocpydecode/third-party/dlpack")
