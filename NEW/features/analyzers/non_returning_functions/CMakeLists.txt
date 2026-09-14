# Each analyzer owns this source and test registration boundary.

add_library(analyzer_non_returning_functions STATIC)
target_sources(analyzer_non_returning_functions PRIVATE src/non_returning_functions.cppm)
target_link_libraries(analyzer_non_returning_functions PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_non_returning_functions_tests)
    target_sources(analyzer_non_returning_functions_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/non_returning_functions_tests.cppm)
    target_link_libraries(analyzer_non_returning_functions_tests PRIVATE analyzer_non_returning_functions analyzer_disassemble_entry_points analyzer_subroutine_references analyzer_function_body NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_non_returning_functions_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_non_returning_functions_tests COMMAND analyzer_non_returning_functions_tests)
endif()
