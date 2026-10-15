cmake_minimum_required(VERSION 3.18)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(rocpydecodeInstalledTests LANGUAGES NONE)
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
print(int(hasattr(native, "PyRocVideoDecoderCpu")))]=]
            "${ROCPYVIDEO_TEST_PYTHONPATH}"
        RESULT_VARIABLE _import_status OUTPUT_VARIABLE _import_output
        ERROR_VARIABLE _import_error OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _import_status EQUAL 0)
        message(FATAL_ERROR
            "Cannot import the installed rocpydecode bindings from ${ROCPYVIDEO_TEST_PYTHONPATH} "
            "with ${Python3_EXECUTABLE}. Check the Python version, package location, "
            "and runtime library availability.\n${_import_error}")
    endif()
    set(ROCPYVIDEO_ENABLE_HOST "${_import_output}")
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

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    foreach(_media IN ITEMS AMD_driving_virtual_20-H264.264 AMD_driving_virtual_20-H265.265)
        if(NOT EXISTS "${ROCPYVIDEO_MEDIA_DIR}/${_media}")
            message(FATAL_ERROR "Missing ${ROCPYVIDEO_MEDIA_DIR}/${_media}. Install the rocDecode test media or set ROCPYVIDEO_MEDIA_DIR.")
        endif()
    endforeach()
endif()

function(rocpyvideo_add_test test_name)
    add_test(NAME ${test_name} COMMAND ${ARGN})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "PYTHONPATH=${ROCPYVIDEO_TEST_PYTHONPATH}"
        TIMEOUT 300)
endfunction()

# Keep the default suite independent of FFmpeg, the rocDecode host library,
# PyTorch, hip-python, and other optional sample dependencies.
rocpyvideo_add_test(rocpydecode_test_types
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/types_test.py")

if(EXISTS "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H264.264")
    rocpyvideo_add_test(video_decode_raw_python
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../samples/rocdecode/videodecoderaw.py"
        -i "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H264.264")
else()
    message(STATUS "H.264 raw test media unavailable; skipping raw H.264 CTest")
endif()

if(EXISTS "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H265.265")
    rocpyvideo_add_test(video_decode_raw_python_h265
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../samples/rocdecode/videodecoderaw.py"
        -i "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H265.265" --codec h265)
else()
    message(STATUS "H.265 raw test media unavailable; skipping raw H.265 CTest")
endif()

if(EXISTS "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H264.264" AND
   EXISTS "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H265.265")
    rocpyvideo_add_test(video_decode_raw_regressions
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/raw_regression_test.py"
        --media-dir "${ROCPYVIDEO_MEDIA_DIR}")
endif()

# Exercise optional API smoke tests only when their backend and media exist.
set(_rocpyvideo_api_media "${ROCPYVIDEO_MEDIA_DIR}/AMD_driving_virtual_20-H264.mp4")
if(ROCPYVIDEO_ENABLE_HOST AND EXISTS "${_rocpyvideo_api_media}")
    rocpyvideo_add_test(video_decode_debug_api
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/decoder_api_test.py"
        -i "${_rocpyvideo_api_media}")
    set_tests_properties(video_decode_debug_api PROPERTIES SKIP_RETURN_CODE 77)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import numpy"
        RESULT_VARIABLE _rocpyvideo_numpy_status OUTPUT_QUIET ERROR_QUIET)
    if(_rocpyvideo_numpy_status EQUAL 0)
        rocpyvideo_add_test(video_decode_cpu_api
            "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/decodercpu_test.py"
            -i "${_rocpyvideo_api_media}")
    else()
        message(STATUS "NumPy unavailable; skipping optional CPU API smoke test")
    endif()
endif()
