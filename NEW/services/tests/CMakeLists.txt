if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(new_ghidra_service_tests)
    target_sources(new_ghidra_service_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES service_tests.cppm
    )
    target_link_libraries(new_ghidra_service_tests PRIVATE
        NewGhidra::PeLoaderService
        NewGhidra::Sleigh
        NewGhidra::FunctionIdService
        NewGhidra::DecompilerService
        GTest::gtest_main
    )
    target_compile_definitions(new_ghidra_service_tests PRIVATE
        NEW_GHIDRA_SERVICE_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../.."
    )
    target_compile_features(new_ghidra_service_tests PRIVATE cxx_std_23)
    add_test(NAME new_ghidra_service_tests COMMAND new_ghidra_service_tests)
endif()
