find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB SLEIGH_RUNTIME_MODULES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
)

if(MSVC)
    get_filename_component(SLEIGH_MSVC_ARCH_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(SLEIGH_MSVC_HOST_BIN "${SLEIGH_MSVC_ARCH_BIN}" DIRECTORY)
    get_filename_component(SLEIGH_MSVC_BIN "${SLEIGH_MSVC_HOST_BIN}" DIRECTORY)
    get_filename_component(SLEIGH_MSVC_TOOLSET "${SLEIGH_MSVC_BIN}" DIRECTORY)
    set(SLEIGH_STD_MODULE "${SLEIGH_MSVC_TOOLSET}/modules/std.ixx")
    if(NOT EXISTS "${SLEIGH_STD_MODULE}")
        message(FATAL_ERROR "MSVC std module was not found at ${SLEIGH_STD_MODULE}")
    endif()
endif()

add_library(sleigh_runtime STATIC)
target_sources(sleigh_runtime
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES sleigh_runtime.cppm
    PRIVATE
        sleigh_runtime_adapter.cppm
    PRIVATE
        FILE_SET runtime_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
        FILES ${SLEIGH_RUNTIME_MODULES}
)

if(MSVC)
    target_sources(sleigh_runtime
        PUBLIC
            FILE_SET msvc_std_modules TYPE CXX_MODULES
            BASE_DIRS "${SLEIGH_MSVC_TOOLSET}/modules"
            FILES "${SLEIGH_STD_MODULE}"
    )
endif()

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
