add_executable(sleigh_runtime_tests sleigh_runtime_tests.cpp)
target_link_libraries(sleigh_runtime_tests PRIVATE NewGhidra::SleighRuntime GTest::gtest_main)
target_compile_features(sleigh_runtime_tests PRIVATE cxx_std_23)
target_compile_definitions(sleigh_runtime_tests PRIVATE
    SLEIGH_RUNTIME_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test_data"
)

add_test(NAME sleigh_runtime_tests COMMAND sleigh_runtime_tests)
