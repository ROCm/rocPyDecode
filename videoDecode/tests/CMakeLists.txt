cmake_minimum_required(VERSION 3.18)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(rocpydecodeInstalledTests LANGUAGES NONE)
    enable_testing()
    # Minimum version, not an exact pin: Python 3.12+ is accepted.
    # The interpreter must match the installed extension's Python ABI.
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

    if(NOT DEFINED ROCM_PATH)
        if(DEFINED ENV{ROCM_PATH})
            set(ROCM_PATH "$ENV{ROCM_PATH}")
        else()
            set(ROCM_PATH /opt/rocm)
        endif()
    endif()
    set(ROCM_PATH "${ROCM_PATH}" CACHE PATH "ROCm runtime and test media prefix")

    # This relative path is filled in when installing this CMakeLists.txt.
    get_filename_component(_bindings_dir
        "${CMAKE_CURRENT_SOURCE_DIR}/@ROCPYVIDEO_TEST_LIBDIR@" ABSOLUTE)
    set(ROCPYVIDEO_TEST_PYTHONPATH "${_bindings_dir}" CACHE PATH
        "Directory containing the installed rocpydecode extension and Python wrapper")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "PYTHONPATH=${ROCPYVIDEO_TEST_PYTHONPATH}" "${Python3_EXECUTABLE}" -c
            [=[import pathlib, sys
import rocpydecode as native
import pyRocVideoDecode as wrapper
expected = pathlib.Path(sys.argv[1]).resolve()
native_path = pathlib.Path(native.__file__).resolve()
wrapper_path = pathlib.Path(wrapper.__file__).resolve()
if native_path.parent != expected or wrapper_path.parent.parent != expected:
    raise ImportError(
        f"Expected bindings under {expected}, but imported native extension "
        f"{native_path} and Python wrapper {wrapper_path}. "
        "Tests must use the selected installation, not another installed copy.")
]=]
            "${ROCPYVIDEO_TEST_PYTHONPATH}"
        RESULT_VARIABLE _import_status OUTPUT_VARIABLE _import_output
        ERROR_VARIABLE _import_error OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _import_status EQUAL 0)
        message(FATAL_ERROR
            "Cannot import the installed rocpydecode bindings from ${ROCPYVIDEO_TEST_PYTHONPATH} "
            "with ${Python3_EXECUTABLE}. Check the Python version, package location, "
            "and runtime library availability.\n${_import_error}")
    endif()
else()
    set(ROCPYVIDEO_TEST_PYTHONPATH
        "${ROCPYVIDEO_OUTPUT_DIR}/lib:${ROCPYVIDEO_OUTPUT_DIR}:$ENV{PYTHONPATH}")
    # Keep the installed suite relocatable, including custom lib/data directories.
    file(RELATIVE_PATH ROCPYVIDEO_TEST_LIBDIR
        "${CMAKE_INSTALL_FULL_DATADIR}/rocpydecode/tests" "${CMAKE_INSTALL_FULL_LIBDIR}")
    configure_file(CMakeLists.txt installed/CMakeLists.txt @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/installed/CMakeLists.txt"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/rocpydecode/tests")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION
        "${CMAKE_INSTALL_DATADIR}/rocpydecode/tests"
        FILES_MATCHING PATTERN "*.py")
endif()
set(ROCPYVIDEO_MEDIA_DIR "${ROCM_PATH}/share/rocdecode/video" CACHE PATH
    "Directory containing rocpydecode test media")

function(rocpyvideo_add_test test_name)
    add_test(NAME ${test_name} COMMAND ${ARGN})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "PYTHONPATH=${ROCPYVIDEO_TEST_PYTHONPATH}"
        TIMEOUT 300)
endfunction()

rocpyvideo_add_test(rocpydecode_test_types
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/types_test.py")

# Check optional dependencies at runtime so installing them needs no reconfigure.
set(_rocpyvideo_api_media "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H264.mp4")
rocpyvideo_add_test(video_decode_api
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/decoder_api_test.py"
    -i "${_rocpyvideo_api_media}")
rocpyvideo_add_test(video_decode_pyav_regressions
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/pyav_regression_test.py"
    -i "${_rocpyvideo_api_media}")
set_tests_properties(video_decode_api video_decode_pyav_regressions
    PROPERTIES SKIP_RETURN_CODE 77)
