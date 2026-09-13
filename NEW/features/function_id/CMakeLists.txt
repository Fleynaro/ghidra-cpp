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

add_executable(function_id_cli cli/function_id_cli.cppm)
target_link_libraries(function_id_cli PRIVATE NewGhidra::FunctionId NewGhidra::SleighRuntime)
target_compile_features(function_id_cli PRIVATE cxx_std_23)
target_compile_definitions(function_id_cli PRIVATE FUNCTION_ID_DATABASE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(MSVC)
    target_compile_options(function_id PRIVATE /W3)
else()
    target_compile_options(function_id PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
