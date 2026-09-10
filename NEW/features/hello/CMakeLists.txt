add_library(hello_feature STATIC)
target_sources(hello_feature
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES hello.cppm
)
add_library(NewGhidra::Hello ALIAS hello_feature)

target_compile_features(hello_feature PUBLIC cxx_std_23)
target_include_directories(hello_feature PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(tests)
