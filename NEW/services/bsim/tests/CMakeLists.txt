find_package(GTest CONFIG REQUIRED)

add_executable(bsim_fixture data/bsim_fixture.cpp)
target_compile_features(bsim_fixture PRIVATE cxx_std_23)
set_target_properties(bsim_fixture PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")

add_executable(bsim_tests bsim_tests.cppm)
target_compile_features(bsim_tests PRIVATE cxx_std_23)
target_compile_definitions(bsim_tests PRIVATE BSIM_FIXTURE_PATH="$<TARGET_FILE:bsim_fixture>")
target_link_libraries(bsim_tests PRIVATE ReCode::Bsim GTest::gtest_main)
add_dependencies(bsim_tests bsim_fixture)

add_executable(bsim_similarity_integration_tests)
target_sources(bsim_similarity_integration_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES
    FILES bsim_similarity_integration_tests.cppm)
target_compile_features(bsim_similarity_integration_tests PRIVATE cxx_std_23)
target_compile_definitions(bsim_similarity_integration_tests PRIVATE BSIM_FIXTURE_PATH="$<TARGET_FILE:bsim_fixture>")
target_link_libraries(bsim_similarity_integration_tests PRIVATE ReCode::Bsim ReCode::PeLoader ReCode::DecompilerFrontend
                                                                   GTest::gtest_main)
add_dependencies(bsim_similarity_integration_tests bsim_fixture)

add_test(NAME bsim_tests COMMAND bsim_tests)
set_tests_properties(bsim_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_test(NAME bsim_similarity_integration_tests COMMAND bsim_similarity_integration_tests)
set_tests_properties(bsim_similarity_integration_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")
