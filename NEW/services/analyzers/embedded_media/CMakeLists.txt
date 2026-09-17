# Each analyzer owns this source and test registration boundary.

add_library(analyzer_embedded_media STATIC)
target_sources(analyzer_embedded_media PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/embedded_media.cppm)
target_link_libraries(analyzer_embedded_media PUBLIC ReCode::AnalyzerShared)
target_compile_features(analyzer_embedded_media PUBLIC cxx_std_23)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_embedded_media_tests)
    target_sources(analyzer_embedded_media_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/embedded_media_tests.cppm)
    target_link_libraries(analyzer_embedded_media_tests PRIVATE analyzer_embedded_media ReCode::AnalyzerTestSupport GTest::gtest_main)
    target_compile_features(analyzer_embedded_media_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_embedded_media_tests COMMAND analyzer_embedded_media_tests)
endif()
