add_executable(analyzer_tests)
target_sources(analyzer_tests
    PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES analyzer_tests.cppm
)
target_link_libraries(analyzer_tests PRIVATE NewGhidra::Analyzer GTest::gtest_main)
target_compile_features(analyzer_tests PRIVATE cxx_std_23)
target_compile_definitions(analyzer_tests PRIVATE
    ANALYZER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test_data"
)
if(MSVC)
    target_compile_options(analyzer_tests PRIVATE /W3)
else()
    target_compile_options(analyzer_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME analyzer_tests COMMAND analyzer_tests)
