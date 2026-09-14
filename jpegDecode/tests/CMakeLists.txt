set(ROCPYJPEG_TEST_PYTHONPATH
    "${ROCPYJPEG_OUTPUT_DIR}/lib:${ROCPYJPEG_OUTPUT_DIR}:$ENV{PYTHONPATH}")
set(ROCPYJPEG_MEDIA_DIR "${ROCM_PATH}/share/rocjpeg/images")

function(rocpyjpeg_add_test test_name)
    add_test(NAME ${test_name} COMMAND ${ARGN})
    set(test_environment "PYTHONPATH=${ROCPYJPEG_TEST_PYTHONPATH}")
    if(EXISTS "${ROCM_PATH}/lib/rocm_sysdeps/lib")
        list(APPEND test_environment "LD_LIBRARY_PATH=${ROCM_PATH}/lib:$ENV{LD_LIBRARY_PATH}")
    endif()
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "${test_environment}"
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
else()
    message(STATUS "rocJPEG *.jpg test images unavailable; JPEG runtime tests are not registered")
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION
    "${CMAKE_INSTALL_DATADIR}/rocpyjpegdecode/tests"
    FILES_MATCHING PATTERN "*.py")
