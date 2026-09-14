# Each analyzer owns this source and test registration boundary.

add_library(analyzer_data_reference STATIC)
target_sources(analyzer_data_reference PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/data_reference.cppm)
target_link_libraries(analyzer_data_reference PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_data_reference_tests)
    target_sources(analyzer_data_reference_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/data_reference_tests.cppm)
    target_link_libraries(analyzer_data_reference_tests PRIVATE analyzer_data_reference analyzer_reference analyzer_constant_propagation analyzer_subroutine_references analyzer_function_body NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_data_reference_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_data_reference_tests COMMAND analyzer_data_reference_tests)
endif()
