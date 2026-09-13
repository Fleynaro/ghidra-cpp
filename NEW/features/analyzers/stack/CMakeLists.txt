# Each analyzer owns this source and test registration boundary.

add_library(analyzer_stack STATIC)
target_sources(analyzer_stack PRIVATE src/stack.cppm)
target_link_libraries(analyzer_stack PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_stack_tests)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." ANALYZER_ROOT)
    target_sources(analyzer_stack_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/stack_tests.cppm)
    target_link_libraries(analyzer_stack_tests PRIVATE analyzer_stack analyzer_disassemble_entry_points analyzer_subroutine_references analyzer_function_body GTest::gtest_main)
    target_compile_features(analyzer_stack_tests PRIVATE cxx_std_23)
    target_compile_definitions(analyzer_stack_tests PRIVATE ANALYZER_FIXTURE_DIR="${ANALYZER_ROOT}")
    add_test(NAME analyzer_stack_tests COMMAND analyzer_stack_tests)
endif()
