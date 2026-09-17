if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(recode_runtime_tests)
    target_sources(recode_runtime_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES runtime_tests.cppm
    )
    target_link_libraries(recode_runtime_tests PRIVATE ReCode::Runtime GTest::gtest_main)
    target_compile_features(recode_runtime_tests PRIVATE cxx_std_23)
    add_test(NAME recode_runtime_tests COMMAND recode_runtime_tests)
endif()
