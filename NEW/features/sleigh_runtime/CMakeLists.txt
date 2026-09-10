find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB SLEIGH_REFERENCE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ghidra/*.cc"
)

add_library(sleigh_runtime STATIC)
target_sources(sleigh_runtime
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES sleigh_runtime.cppm
    PRIVATE
        src/sleigh_runtime.cpp
        ${SLEIGH_REFERENCE_SOURCES}
)

add_library(NewGhidra::SleighRuntime ALIAS sleigh_runtime)

target_compile_features(sleigh_runtime PUBLIC cxx_std_23)
target_include_directories(sleigh_runtime
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/ghidra"
)
target_link_libraries(sleigh_runtime PRIVATE ZLIB::ZLIB)

if(MSVC)
    target_compile_options(sleigh_runtime PRIVATE /W3)
else()
    target_compile_options(sleigh_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(tests)
