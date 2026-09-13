# Each analyzer owns this source and test registration boundary.

add_library(analyzer_reference STATIC)
target_sources(analyzer_reference PRIVATE src/reference.cppm)
target_link_libraries(analyzer_reference PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_reference_tests)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." ANALYZER_ROOT)
    target_sources(analyzer_reference_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/reference_tests.cppm)
    target_link_libraries(analyzer_reference_tests PRIVATE analyzer_reference analyzer_disassemble_entry_points analyzer_constant_propagation analyzer_subroutine_references analyzer_function_body GTest::gtest_main)
    target_compile_features(analyzer_reference_tests PRIVATE cxx_std_23)
    target_compile_definitions(analyzer_reference_tests PRIVATE ANALYZER_FIXTURE_DIR="${ANALYZER_ROOT}")
    add_test(NAME analyzer_reference_tests COMMAND analyzer_reference_tests)
endif()
