set(ROCPYVIDEO_TEST_PYTHONPATH
    "${ROCPYVIDEO_OUTPUT_DIR}/lib:${ROCPYVIDEO_OUTPUT_DIR}:$ENV{PYTHONPATH}")
set(ROCPYVIDEO_MEDIA_DIR "${ROCM_PATH}/share/rocdecode/video")

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

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION
    "${CMAKE_INSTALL_DATADIR}/rocpydecode/tests"
    FILES_MATCHING PATTERN "*.py")
