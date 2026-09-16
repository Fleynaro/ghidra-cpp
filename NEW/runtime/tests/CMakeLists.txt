if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(new_ghidra_runtime_tests)
    target_sources(new_ghidra_runtime_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES runtime_tests.cppm
    )
    target_link_libraries(new_ghidra_runtime_tests PRIVATE NewGhidra::Runtime GTest::gtest_main)
    target_compile_features(new_ghidra_runtime_tests PRIVATE cxx_std_23)
    add_test(NAME new_ghidra_runtime_tests COMMAND new_ghidra_runtime_tests)
endif()
