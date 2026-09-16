# Each analyzer owns this source and test registration boundary.
add_library(analyzer_decompiler_parameter_id STATIC)
target_sources(analyzer_decompiler_parameter_id PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/decompiler_parameter_id.cppm)
target_link_libraries(analyzer_decompiler_parameter_id PUBLIC NewGhidra::AnalyzerShared NewGhidra::DecompilerFrontend)
target_compile_features(analyzer_decompiler_parameter_id PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_decompiler_parameter_id_tests)
    target_sources(analyzer_decompiler_parameter_id_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/decompiler_parameter_id_tests.cppm)
    target_link_libraries(analyzer_decompiler_parameter_id_tests PRIVATE analyzer_decompiler_parameter_id NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_decompiler_parameter_id_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_decompiler_parameter_id_tests COMMAND analyzer_decompiler_parameter_id_tests)
endif()
