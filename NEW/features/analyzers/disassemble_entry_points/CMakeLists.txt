# Each analyzer owns this source and test registration boundary.

add_library(analyzer_disassemble_entry_points STATIC)
target_sources(analyzer_disassemble_entry_points PRIVATE src/disassemble_entry_points.cppm)
target_link_libraries(analyzer_disassemble_entry_points PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_disassemble_entry_points_tests)
    target_sources(analyzer_disassemble_entry_points_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/disassemble_entry_points_tests.cppm)
    target_link_libraries(analyzer_disassemble_entry_points_tests PRIVATE analyzer_disassemble_entry_points NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_disassemble_entry_points_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_disassemble_entry_points_tests COMMAND analyzer_disassemble_entry_points_tests)
endif()
