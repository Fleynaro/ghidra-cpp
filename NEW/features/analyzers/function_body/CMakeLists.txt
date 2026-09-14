# This directory contains CreateFunctionCmd/FollowFlow integration tests.  The
# original Ghidra architecture has no FunctionBodyAnalyzer; body construction
# is implemented synchronously by AnalysisContext::create_function().

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_function_body_tests)
    target_sources(analyzer_function_body_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/function_body_tests.cppm)
    target_link_libraries(analyzer_function_body_tests PRIVATE NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_function_body_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_function_body_tests COMMAND analyzer_function_body_tests)
endif()
