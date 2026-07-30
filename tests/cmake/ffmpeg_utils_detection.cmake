if(NOT DEFINED ROCPYDECODE_SOURCE_DIR)
    message(FATAL_ERROR "ROCPYDECODE_SOURCE_DIR must be set")
endif()

if(NOT DEFINED ROCPYDECODE_TEST_BINARY_DIR)
    set(ROCPYDECODE_TEST_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg_utils_detection")
endif()

include("${ROCPYDECODE_SOURCE_DIR}/cmake/rocPyDecodeFFmpegUtils.cmake")

set(_rocm_root "${ROCPYDECODE_TEST_BINARY_DIR}/fake_rocm")
set(_utils_dir "${_rocm_root}/share/rocdecode/utils/ffmpegvideodecode")
file(REMOVE_RECURSE "${_rocm_root}")

rocpydecode_check_ffmpeg_utils("${_rocm_root}" _found _sources _headers _reported_dir)
if(_found)
    message(FATAL_ERROR "Expected rocdecode FFmpeg utilities to be absent for an empty ROCm root")
endif()

file(MAKE_DIRECTORY "${_utils_dir}")
file(WRITE "${_utils_dir}/ffmpeg_video_dec.h" "/* test header */\n")

rocpydecode_check_ffmpeg_utils("${_rocm_root}" _found _sources _headers _reported_dir)
if(_found)
    message(FATAL_ERROR "Expected rocdecode FFmpeg utilities to be absent with only the required header")
endif()

file(WRITE "${_utils_dir}/ffmpeg_video_dec.cpp" "/* test source */\n")

rocpydecode_check_ffmpeg_utils("${_rocm_root}" _found _sources _headers _reported_dir)
if(NOT _found)
    message(FATAL_ERROR "Expected rocdecode FFmpeg utilities to be detected with header and source files")
endif()
