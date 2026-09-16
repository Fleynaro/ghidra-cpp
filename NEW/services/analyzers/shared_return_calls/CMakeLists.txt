# Each analyzer owns this source and test registration boundary.

add_library(analyzer_shared_return_calls STATIC)
target_sources(analyzer_shared_return_calls PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/shared_return_calls.cppm)
target_link_libraries(analyzer_shared_return_calls PUBLIC NewGhidra::AnalyzerShared)
target_compile_features(analyzer_shared_return_calls PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_shared_return_calls_tests)
    target_sources(analyzer_shared_return_calls_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/shared_return_calls_tests.cppm)
    target_link_libraries(analyzer_shared_return_calls_tests PRIVATE analyzer_shared_return_calls NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_shared_return_calls_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_shared_return_calls_tests COMMAND analyzer_shared_return_calls_tests)
endif()
