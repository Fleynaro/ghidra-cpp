if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(new_ghidra_project_tests)
    target_sources(new_ghidra_project_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES project_tests.cppm
    )
    target_link_libraries(new_ghidra_project_tests PRIVATE NewGhidra::RuntimeProject GTest::gtest_main)
    target_compile_features(new_ghidra_project_tests PRIVATE cxx_std_23)
    target_compile_definitions(new_ghidra_project_tests PRIVATE
        NEW_GHIDRA_PROJECT_FIXTURE_DIR="${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME new_ghidra_project_tests COMMAND new_ghidra_project_tests)
endif()
