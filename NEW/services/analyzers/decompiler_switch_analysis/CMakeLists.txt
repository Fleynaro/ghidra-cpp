# Each analyzer owns this source and test registration boundary.
add_library(analyzer_decompiler_switch_analysis STATIC)
target_sources(analyzer_decompiler_switch_analysis PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/decompiler_switch_analysis.cppm)
target_link_libraries(analyzer_decompiler_switch_analysis PUBLIC ReCode::AnalyzerShared ReCode::DecompilerFrontend)
target_compile_features(analyzer_decompiler_switch_analysis PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_decompiler_switch_analysis_tests)
    target_sources(analyzer_decompiler_switch_analysis_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/decompiler_switch_analysis_tests.cppm)
    target_link_libraries(analyzer_decompiler_switch_analysis_tests PRIVATE analyzer_decompiler_switch_analysis ReCode::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_decompiler_switch_analysis_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_decompiler_switch_analysis_tests COMMAND analyzer_decompiler_switch_analysis_tests)
endif()
