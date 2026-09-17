# Each analyzer owns this source and test registration boundary.

add_library(analyzer_aggressive_instruction_finder STATIC)
target_sources(analyzer_aggressive_instruction_finder PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/aggressive_instruction_finder.cppm)
target_link_libraries(analyzer_aggressive_instruction_finder PUBLIC ReCode::AnalyzerShared)
target_compile_features(analyzer_aggressive_instruction_finder PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_aggressive_instruction_finder_tests)
    target_sources(analyzer_aggressive_instruction_finder_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/aggressive_instruction_finder_tests.cppm)
    target_link_libraries(analyzer_aggressive_instruction_finder_tests PRIVATE analyzer_aggressive_instruction_finder ReCode::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_aggressive_instruction_finder_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_aggressive_instruction_finder_tests COMMAND analyzer_aggressive_instruction_finder_tests)
endif()
