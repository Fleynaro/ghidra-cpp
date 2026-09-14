# Each analyzer owns this source and test registration boundary.

add_library(analyzer_function_start_search STATIC)
target_sources(analyzer_function_start_search PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/function_start_search.cppm)
target_link_libraries(analyzer_function_start_search PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_function_start_search_tests)
    target_sources(analyzer_function_start_search_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/function_start_search_tests.cppm)
    target_link_libraries(analyzer_function_start_search_tests PRIVATE analyzer_function_start_search analyzer_disassemble_entry_points NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_function_start_search_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_function_start_search_tests COMMAND analyzer_function_start_search_tests)
endif()
