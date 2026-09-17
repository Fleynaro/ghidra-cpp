# Each analyzer owns this source and test registration boundary.

add_library(analyzer_create_address_tables STATIC)
target_sources(analyzer_create_address_tables PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/create_address_tables.cppm)
target_link_libraries(analyzer_create_address_tables PUBLIC ReCode::AnalyzerShared)
target_compile_features(analyzer_create_address_tables PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_create_address_tables_tests)
    target_sources(analyzer_create_address_tables_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/create_address_tables_tests.cppm)
    target_link_libraries(analyzer_create_address_tables_tests PRIVATE analyzer_create_address_tables ReCode::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_create_address_tables_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_create_address_tables_tests COMMAND analyzer_create_address_tables_tests)
endif()
