find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(function_id STATIC)

target_sources(function_id
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES function_id.cppm
    PRIVATE
        src/function_id.cpp
)

add_library(NewGhidra::FunctionId ALIAS function_id)
target_compile_features(function_id PUBLIC cxx_std_23)
target_link_libraries(function_id PUBLIC NewGhidra::SleighRuntime PRIVATE ZLIB::ZLIB)

if(MSVC)
    target_compile_options(function_id PRIVATE /W3)
else()
    target_compile_options(function_id PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(tests)
