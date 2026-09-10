find_package(GTest CONFIG REQUIRED)

add_executable(hello_feature_tests hello_tests.cpp)
target_link_libraries(hello_feature_tests PRIVATE hello_feature GTest::gtest_main)
target_compile_features(hello_feature_tests PRIVATE cxx_std_23)

add_test(NAME hello_feature_tests COMMAND hello_feature_tests)
