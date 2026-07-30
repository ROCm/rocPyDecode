function(rocpydecode_check_video_utils ROCM_ROOT RESULT_VAR SOURCES_VAR HEADERS_VAR UTILS_DIR_VAR)
    set(_utils_dir "${ROCM_ROOT}/share/rocdecode/utils/rocvideodecode")
    set(_required_header "${_utils_dir}/roc_video_dec.h")

    file(GLOB _sources "${_utils_dir}/*.cpp")
    file(GLOB _headers "${_utils_dir}/*.h")

    if(EXISTS "${_required_header}" AND _sources)
        set(_found TRUE)
    else()
        set(_found FALSE)
    endif()

    set(${RESULT_VAR} ${_found} PARENT_SCOPE)
    set(${SOURCES_VAR} ${_sources} PARENT_SCOPE)
    set(${HEADERS_VAR} ${_headers} PARENT_SCOPE)
    set(${UTILS_DIR_VAR} "${_utils_dir}" PARENT_SCOPE)
endfunction()

function(rocpydecode_check_ffmpeg_utils ROCM_ROOT RESULT_VAR SOURCES_VAR HEADERS_VAR UTILS_DIR_VAR)
    set(_utils_dir "${ROCM_ROOT}/share/rocdecode/utils/ffmpegvideodecode")
    set(_required_header "${_utils_dir}/ffmpeg_video_dec.h")

    file(GLOB _sources "${_utils_dir}/*.cpp")
    file(GLOB _headers "${_utils_dir}/*.h")

    if(EXISTS "${_required_header}" AND _sources)
        set(_found TRUE)
    else()
        set(_found FALSE)
    endif()

    set(${RESULT_VAR} ${_found} PARENT_SCOPE)
    set(${SOURCES_VAR} ${_sources} PARENT_SCOPE)
    set(${HEADERS_VAR} ${_headers} PARENT_SCOPE)
    set(${UTILS_DIR_VAR} "${_utils_dir}" PARENT_SCOPE)
endfunction()
