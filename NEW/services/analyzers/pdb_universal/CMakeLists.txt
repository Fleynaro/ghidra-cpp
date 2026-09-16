# Each analyzer owns this source and test registration boundary.
add_library(analyzer_pdb_universal STATIC)
target_sources(analyzer_pdb_universal PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pdb_universal.cppm)
target_link_libraries(analyzer_pdb_universal PUBLIC NewGhidra::AnalyzerShared)
target_compile_features(analyzer_pdb_universal PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_pdb_universal_tests)
    target_sources(analyzer_pdb_universal_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/pdb_universal_tests.cppm)
    target_link_libraries(analyzer_pdb_universal_tests PRIVATE analyzer_pdb_universal NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_pdb_universal_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_pdb_universal_tests COMMAND analyzer_pdb_universal_tests)
endif()
