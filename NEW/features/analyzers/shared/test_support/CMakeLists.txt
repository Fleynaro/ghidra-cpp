if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_library(analyzer_test_support STATIC)
    add_library(NewGhidra::AnalyzerTestSupport ALIAS analyzer_test_support)
    target_sources(analyzer_test_support PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES analyzer_test_support.cppm
    )
    target_compile_features(analyzer_test_support PUBLIC cxx_std_23)
    target_compile_definitions(analyzer_test_support PUBLIC ANALYZER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../..")
    target_link_libraries(analyzer_test_support PUBLIC NewGhidra::AnalyzerShared GTest::gtest)
endif()
