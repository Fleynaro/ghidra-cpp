find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB SLEIGH_GHIDRA_MODULES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ghidra/*.cppm"
)

add_library(sleigh_runtime STATIC)
target_sources(sleigh_runtime
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES sleigh_runtime.cppm
    PRIVATE
        sleigh_runtime_adapter.cppm
    PRIVATE
        FILE_SET ghidra_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/ghidra"
        FILES ${SLEIGH_GHIDRA_MODULES}
)

add_library(NewGhidra::SleighRuntime ALIAS sleigh_runtime)

target_compile_features(sleigh_runtime PUBLIC cxx_std_23)
target_link_libraries(sleigh_runtime PRIVATE ZLIB::ZLIB)

if(MSVC)
    target_compile_options(sleigh_runtime PRIVATE /W3)
else()
    target_compile_options(sleigh_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(sleigh_runtime_decode cli/sleigh_runtime_decode.cppm)
target_link_libraries(sleigh_runtime_decode PRIVATE NewGhidra::SleighRuntime)
target_compile_features(sleigh_runtime_decode PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(sleigh_runtime_decode PRIVATE /W3)
else()
    target_compile_options(sleigh_runtime_decode PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(tests)
