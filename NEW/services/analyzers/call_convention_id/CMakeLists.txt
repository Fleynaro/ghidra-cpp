# Each analyzer owns this source and test registration boundary.
add_library(analyzer_call_convention_id STATIC)
target_sources(analyzer_call_convention_id PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/call_convention_id.cppm)
target_link_libraries(analyzer_call_convention_id PUBLIC ReCode::AnalyzerShared ReCode::DecompilerFrontend)
target_compile_features(analyzer_call_convention_id PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_call_convention_id_tests)
    target_sources(analyzer_call_convention_id_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/call_convention_id_tests.cppm)
    target_link_libraries(analyzer_call_convention_id_tests PRIVATE analyzer_call_convention_id ReCode::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_call_convention_id_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_call_convention_id_tests COMMAND analyzer_call_convention_id_tests)
endif()
