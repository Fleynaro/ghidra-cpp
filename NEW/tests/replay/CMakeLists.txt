if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(architecture_replay_tests)
    target_sources(architecture_replay_tests PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES replay_tests.cppm
    )
    target_link_libraries(architecture_replay_tests PRIVATE ReCode::Runtime GTest::gtest_main)
    target_compile_features(architecture_replay_tests PRIVATE cxx_std_23)
    add_test(NAME architecture_replay_tests COMMAND architecture_replay_tests)
endif()
