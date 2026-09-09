# DLPack 1.3 headers are vendored under the Apache-2.0 license.
add_library(dlpack INTERFACE)
target_include_directories(dlpack INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
add_library(dlpack::dlpack ALIAS dlpack)
