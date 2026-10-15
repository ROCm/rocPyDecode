cmake_minimum_required(VERSION 3.18)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(rocpyjpegdecodeInstalledTests LANGUAGES NONE)
    enable_testing()
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
        "${CMAKE_CURRENT_SOURCE_DIR}/@ROCPYJPEG_TEST_LIBDIR@" ABSOLUTE)
    set(ROCPYJPEG_TEST_PYTHONPATH "${_bindings_dir}" CACHE PATH
        "Directory containing the installed rocpyjpegdecode extension and Python wrapper")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "PYTHONPATH=${ROCPYJPEG_TEST_PYTHONPATH}" "${Python3_EXECUTABLE}" -c
            [=[import pathlib, sys
import rocpyjpegdecode as native
import pyRocJpegDecode as wrapper
expected = pathlib.Path(sys.argv[1]).resolve()
native_path = pathlib.Path(native.__file__).resolve()
wrapper_path = pathlib.Path(wrapper.__file__).resolve()
if native_path.parent != expected or wrapper_path.parent.parent != expected:
    raise ImportError(
        f"Expected bindings under {expected}, but imported native extension "
        f"{native_path} and Python wrapper {wrapper_path}. "
        "Tests must use the selected installation, not another installed copy.")
]=]
            "${ROCPYJPEG_TEST_PYTHONPATH}"
        RESULT_VARIABLE _import_status OUTPUT_VARIABLE _import_output
        ERROR_VARIABLE _import_error OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _import_status EQUAL 0)
        message(FATAL_ERROR
            "Cannot import the installed rocpyjpegdecode bindings from ${ROCPYJPEG_TEST_PYTHONPATH} "
            "with ${Python3_EXECUTABLE}. Check the Python version, package location, "
            "and runtime library availability.\n${_import_error}")
    endif()
else()
    set(ROCPYJPEG_TEST_PYTHONPATH
        "${ROCPYJPEG_OUTPUT_DIR}/lib:${ROCPYJPEG_OUTPUT_DIR}:$ENV{PYTHONPATH}")
    # Keep the installed suite relocatable, including custom lib/data directories.
    file(RELATIVE_PATH ROCPYJPEG_TEST_LIBDIR
        "${CMAKE_INSTALL_FULL_DATADIR}/rocpyjpegdecode/tests" "${CMAKE_INSTALL_FULL_LIBDIR}")
    configure_file(CMakeLists.txt installed/CMakeLists.txt @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/installed/CMakeLists.txt"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/rocpyjpegdecode/tests")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION
        "${CMAKE_INSTALL_DATADIR}/rocpyjpegdecode/tests"
        FILES_MATCHING PATTERN "*.py")
endif()
set(ROCPYJPEG_MEDIA_DIR "${ROCM_PATH}/share/rocjpeg/images" CACHE PATH
    "Directory containing rocpyjpegdecode test media")

function(rocpyjpeg_add_test test_name)
    add_test(NAME ${test_name} COMMAND ${ARGN})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "PYTHONPATH=${ROCPYJPEG_TEST_PYTHONPATH}"
        TIMEOUT 300)
endfunction()

# Match the regression test's top-level *.jpg fixture discovery.
file(GLOB ROCPYJPEG_TEST_IMAGES LIST_DIRECTORIES false
    "${ROCPYJPEG_MEDIA_DIR}/*.jpg")
if(ROCPYJPEG_TEST_IMAGES)
    rocpyjpeg_add_test(jpeg_decode_batched_python
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../samples/rocjpeg/jpegdecodebatched.py"
        -i "${ROCPYJPEG_MEDIA_DIR}")
    rocpyjpeg_add_test(jpeg_decode_regressions
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg_regression_test.py"
        --media-dir "${ROCPYJPEG_MEDIA_DIR}")
    rocpyjpeg_add_test(jpeg_decode_input_regressions
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg_input_test.py"
        --media-dir "${ROCPYJPEG_MEDIA_DIR}")
elseif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "No *.jpg test images in ${ROCPYJPEG_MEDIA_DIR}. Install the rocJPEG test media or set ROCPYJPEG_MEDIA_DIR.")
else()
    message(STATUS "rocJPEG *.jpg test images unavailable; JPEG runtime tests are not registered")
endif()
