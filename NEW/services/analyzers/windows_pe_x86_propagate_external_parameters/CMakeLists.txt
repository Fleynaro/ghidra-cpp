add_library(analyzer_winparams STATIC)
add_library(analyzer_windows_pe_x86_propagate_external_parameters ALIAS analyzer_winparams)
target_sources(analyzer_winparams PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/params.cppm)
target_link_libraries(analyzer_winparams PUBLIC NewGhidra::AnalyzerShared)
target_compile_features(analyzer_winparams PUBLIC cxx_std_23)
# Keep MSVC's compiler-PDB path below the Windows path limit for this long module directory.
set_target_properties(analyzer_winparams PROPERTIES
    COMPILE_PDB_NAME analyzer_params
    COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb")

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_winparams_tests)
    target_sources(analyzer_winparams_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/params_tests.cppm)
    target_link_libraries(analyzer_winparams_tests PRIVATE analyzer_winparams analyzer_reference NewGhidra::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_winparams_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_windows_pe_x86_propagate_external_parameters_tests COMMAND analyzer_winparams_tests)
endif()
