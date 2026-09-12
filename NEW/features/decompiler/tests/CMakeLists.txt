add_executable(decompiler_tests)
target_sources(decompiler_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES decompiler_tests.cppm
)
target_link_libraries(decompiler_tests PRIVATE
    NewGhidra::DecompilerFrontend
    GTest::gtest_main
)
target_compile_features(decompiler_tests PRIVATE cxx_std_23)

add_test(NAME decompiler_tests COMMAND decompiler_tests)
set_tests_properties(decompiler_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
