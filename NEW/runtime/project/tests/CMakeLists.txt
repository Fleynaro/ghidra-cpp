if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(recode_project_tests)
    target_sources(recode_project_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES project_tests.cppm
    )
    target_link_libraries(recode_project_tests PRIVATE ReCode::RuntimeProject GTest::gtest_main)
    target_compile_features(recode_project_tests PRIVATE cxx_std_23)
    target_compile_definitions(recode_project_tests PRIVATE
        RECODE_PROJECT_FIXTURE_DIR="${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME recode_project_tests COMMAND recode_project_tests)
endif()
