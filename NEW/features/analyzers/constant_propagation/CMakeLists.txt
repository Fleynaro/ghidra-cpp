# Each analyzer owns this source and test registration boundary.

add_library(analyzer_constant_propagation STATIC)
target_sources(analyzer_constant_propagation PRIVATE src/constant_propagation.cppm)
target_link_libraries(analyzer_constant_propagation PUBLIC NewGhidra::AnalyzerShared)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_constant_propagation_tests)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." ANALYZER_ROOT)
    target_sources(analyzer_constant_propagation_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/constant_propagation_tests.cppm)
target_link_libraries(analyzer_constant_propagation_tests PRIVATE analyzer_constant_propagation analyzer_subroutine_references analyzer_function_body GTest::gtest_main)
    target_compile_features(analyzer_constant_propagation_tests PRIVATE cxx_std_23)
    target_compile_definitions(analyzer_constant_propagation_tests PRIVATE ANALYZER_FIXTURE_DIR="${ANALYZER_ROOT}")
    add_test(NAME analyzer_constant_propagation_tests COMMAND analyzer_constant_propagation_tests)
endif()
