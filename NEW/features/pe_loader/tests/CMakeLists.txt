find_package(GTest CONFIG REQUIRED)

add_executable(pe_loader_tests pe_loader_tests.cpp)
target_link_libraries(pe_loader_tests PRIVATE NewGhidra::PeLoader GTest::gtest_main)
target_compile_features(pe_loader_tests PRIVATE cxx_std_23)
target_compile_definitions(pe_loader_tests PRIVATE
    PE_LOADER_TEST_FIXTURE="${CMAKE_CURRENT_LIST_DIR}/../../../../TEST/test.exe"
)

add_test(NAME pe_loader_tests COMMAND pe_loader_tests)
