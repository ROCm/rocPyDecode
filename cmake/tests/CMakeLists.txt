# Keep configure regressions available even when video is skipped or disabled.
# A missing interpreter must not turn an otherwise empty build into an error.
find_package(Python3 3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME rocpydecode_configure_missing_video_utils
        COMMAND "${CMAKE_COMMAND}" -E env
            "ROCPYDECODE_TEST_CMAKE=${CMAKE_COMMAND}"
            "ROCPYDECODE_TEST_CXX=${CMAKE_CXX_COMPILER}"
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_missing_video_utils.py")
    set_tests_properties(rocpydecode_configure_missing_video_utils PROPERTIES
        LABELS "configure" TIMEOUT 120)
else()
    message(WARNING "Python 3 interpreter unavailable; skipping configure regression tests.")
endif()
