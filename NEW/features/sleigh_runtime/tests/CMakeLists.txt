add_executable(sleigh_runtime_tests sleigh_runtime_tests.cppm)
target_link_libraries(sleigh_runtime_tests PRIVATE NewGhidra::SleighRuntime GTest::gtest_main)
target_compile_features(sleigh_runtime_tests PRIVATE cxx_std_23)

add_test(NAME sleigh_runtime_tests COMMAND sleigh_runtime_tests)
set_tests_properties(sleigh_runtime_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
