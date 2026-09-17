find_package(GTest CONFIG REQUIRED)

add_executable(ttd_recorder_tests)
target_sources(ttd_recorder_tests PRIVATE
    FILE_SET cxx_modules TYPE CXX_MODULES FILES win_ttd_tests.cppm
)
target_link_libraries(ttd_recorder_tests PRIVATE NewGhidra::TtdRecorder NewGhidra::Std GTest::gtest_main)
target_compile_features(ttd_recorder_tests PRIVATE cxx_std_23)
add_test(NAME ttd_recorder_tests COMMAND ttd_recorder_tests)
set_tests_properties(ttd_recorder_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.." TIMEOUT 30)
