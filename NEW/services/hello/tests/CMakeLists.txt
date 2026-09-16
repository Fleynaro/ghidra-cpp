find_package(GTest CONFIG REQUIRED)

add_executable(hello_service_tests hello_tests.cpp)
target_link_libraries(hello_service_tests PRIVATE hello_service GTest::gtest_main)
target_compile_features(hello_service_tests PRIVATE cxx_std_23)

add_test(NAME hello_service_tests COMMAND hello_service_tests)
