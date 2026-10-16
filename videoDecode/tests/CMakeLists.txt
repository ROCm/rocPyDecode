set(ROCPYVIDEO_TEST_PYTHONPATH
    "${ROCPYVIDEO_OUTPUT_DIR}/lib:${ROCPYVIDEO_OUTPUT_DIR}:$ENV{PYTHONPATH}")
set(ROCPYVIDEO_MEDIA_DIR "${ROCM_PATH}/share/rocdecode/video")

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

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION
    "${CMAKE_INSTALL_DATADIR}/rocpydecode/tests"
    FILES_MATCHING PATTERN "*.py")
