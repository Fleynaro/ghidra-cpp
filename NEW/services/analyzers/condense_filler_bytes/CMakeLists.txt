# Each analyzer owns this source and test registration boundary.

add_library(analyzer_condense_filler_bytes STATIC)
target_sources(analyzer_condense_filler_bytes PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/condense_filler_bytes.cppm)
target_link_libraries(analyzer_condense_filler_bytes PUBLIC NewGhidra::AnalyzerShared)
target_compile_features(analyzer_condense_filler_bytes PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_condense_filler_bytes_tests)
    target_sources(analyzer_condense_filler_bytes_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/condense_filler_bytes_tests.cppm)
    target_link_libraries(analyzer_condense_filler_bytes_tests PRIVATE analyzer_condense_filler_bytes NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_condense_filler_bytes_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_condense_filler_bytes_tests COMMAND analyzer_condense_filler_bytes_tests)
endif()
