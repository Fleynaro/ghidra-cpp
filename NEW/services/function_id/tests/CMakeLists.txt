find_package(GTest CONFIG REQUIRED)

add_executable(function_id_tests function_id_tests.cpp)
target_link_libraries(function_id_tests PRIVATE ReCode::FunctionId GTest::gtest_main)
target_compile_features(function_id_tests PRIVATE cxx_std_23)
target_compile_definitions(function_id_tests PRIVATE
    FUNCTION_ID_TEST_DIR="${CMAKE_CURRENT_LIST_DIR}/../data"
)

add_test(NAME function_id_tests COMMAND function_id_tests)
