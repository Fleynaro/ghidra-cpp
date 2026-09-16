if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_global_integration_tests)
    target_sources(analyzer_global_integration_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES analyzer_global_integration_tests.cppm
    )
    target_link_libraries(analyzer_global_integration_tests PRIVATE
        analyzer
        NewGhidra::AnalyzerTestSupport
        GTest::gtest_main
    )
    target_compile_features(analyzer_global_integration_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_global_integration_tests COMMAND analyzer_global_integration_tests)
endif()
