# X-Phage Test Suite
# run-pass/      → must compile + exit 0
# compile-fail/  → must fail compilation/runtime (exit != 0)

file(GLOB_RECURSE RUNPASS_TESTS
    "${CMAKE_CURRENT_SOURCE_DIR}/run-pass/*.xp0")

foreach(test_file ${RUNPASS_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(
        NAME    "run-pass/${test_name}"
        COMMAND $<TARGET_FILE:xphage> run ${test_file}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("run-pass/${test_name}"
        PROPERTIES TIMEOUT 30)
endforeach()

file(GLOB_RECURSE FAIL_TESTS
    "${CMAKE_CURRENT_SOURCE_DIR}/compile-fail/*.xp0")

foreach(test_file ${FAIL_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(
        NAME    "compile-fail/${test_name}"
        COMMAND $<TARGET_FILE:xphage> build ${test_file}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties("compile-fail/${test_name}"
        PROPERTIES WILL_FAIL TRUE TIMEOUT 30)
endforeach()
