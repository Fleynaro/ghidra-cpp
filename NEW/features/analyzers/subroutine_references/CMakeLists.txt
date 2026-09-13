# Each analyzer owns this source and test registration boundary.

add_library(analyzer_subroutine_references STATIC)
target_sources(analyzer_subroutine_references PRIVATE src/subroutine_references.cppm)
target_link_libraries(analyzer_subroutine_references PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_subroutine_references_tests)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." ANALYZER_ROOT)
    target_sources(analyzer_subroutine_references_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/subroutine_references_tests.cppm)
    target_link_libraries(analyzer_subroutine_references_tests PRIVATE analyzer_subroutine_references analyzer_disassemble_entry_points analyzer_function_body GTest::gtest_main)
    target_compile_features(analyzer_subroutine_references_tests PRIVATE cxx_std_23)
    target_compile_definitions(analyzer_subroutine_references_tests PRIVATE ANALYZER_FIXTURE_DIR="${ANALYZER_ROOT}")
    add_test(NAME analyzer_subroutine_references_tests COMMAND analyzer_subroutine_references_tests)
endif()
