set(ROCPYJPEG_TEST_PYTHONPATH
    "${ROCPYJPEG_OUTPUT_DIR}/lib:${ROCPYJPEG_OUTPUT_DIR}:$ENV{PYTHONPATH}")
set(ROCPYJPEG_MEDIA_DIR "${ROCM_PATH}/share/rocjpeg/images")

function(rocpyjpeg_add_test test_name)
    add_test(NAME ${test_name} COMMAND ${ARGN})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "PYTHONPATH=${ROCPYJPEG_TEST_PYTHONPATH}"
        TIMEOUT 300)
endfunction()

if(IS_DIRECTORY "${ROCPYJPEG_MEDIA_DIR}")
    rocpyjpeg_add_test(jpeg_decode_batched_python
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../samples/rocjpeg/jpegdecodebatched.py"
        -i "${ROCPYJPEG_MEDIA_DIR}")
else()
    message(STATUS "rocJPEG test images unavailable; JPEG runtime tests are not registered")
endif()
