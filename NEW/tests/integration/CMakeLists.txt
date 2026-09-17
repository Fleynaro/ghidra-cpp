if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(architecture_end_to_end_tests)
    target_sources(architecture_end_to_end_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES end_to_end_tests.cppm
    )
    target_link_libraries(architecture_end_to_end_tests PRIVATE ReCode::RuntimeProject GTest::gtest_main)
    target_compile_features(architecture_end_to_end_tests PRIVATE cxx_std_23)
    target_compile_definitions(architecture_end_to_end_tests PRIVATE
        RECODE_INTEGRATION_ROOT="${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME architecture_end_to_end_tests COMMAND architecture_end_to_end_tests)
endif()
