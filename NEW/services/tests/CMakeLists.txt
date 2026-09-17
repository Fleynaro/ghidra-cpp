if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(recode_service_tests)
    target_sources(recode_service_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES service_tests.cppm
    )
    target_link_libraries(recode_service_tests PRIVATE
        ReCode::PeLoaderService
        ReCode::Sleigh
        ReCode::FunctionIdService
        ReCode::DecompilerService
        GTest::gtest_main
    )
    target_compile_definitions(recode_service_tests PRIVATE
        RECODE_SERVICE_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../.."
    )
    target_compile_features(recode_service_tests PRIVATE cxx_std_23)
    add_test(NAME recode_service_tests COMMAND recode_service_tests)
endif()
