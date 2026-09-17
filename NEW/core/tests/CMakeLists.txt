if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(core_domain_tests)
    target_sources(core_domain_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES domain_tests.cppm event_codec_tests.cppm contract_value_tests.cppm
    )
    target_link_libraries(core_domain_tests PRIVATE ReCode::Core GTest::gtest_main)
    target_compile_features(core_domain_tests PRIVATE cxx_std_23)
    add_test(NAME core_domain_tests COMMAND core_domain_tests)
endif()
