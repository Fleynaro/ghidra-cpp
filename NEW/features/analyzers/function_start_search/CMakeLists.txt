# Each analyzer owns this source and test registration boundary.

add_library(analyzer_function_start_search STATIC)
target_sources(analyzer_function_start_search PRIVATE src/function_start_search.cppm)
target_link_libraries(analyzer_function_start_search PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_function_start_search_tests)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." ANALYZER_ROOT)
    target_sources(analyzer_function_start_search_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/function_start_search_tests.cppm)
    target_link_libraries(analyzer_function_start_search_tests PRIVATE analyzer_function_start_search analyzer_disassemble_entry_points GTest::gtest_main)
    target_compile_features(analyzer_function_start_search_tests PRIVATE cxx_std_23)
    target_compile_definitions(analyzer_function_start_search_tests PRIVATE ANALYZER_FIXTURE_DIR="${ANALYZER_ROOT}")
    add_test(NAME analyzer_function_start_search_tests COMMAND analyzer_function_start_search_tests)
endif()
