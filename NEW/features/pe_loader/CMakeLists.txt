add_library(pe_loader STATIC)

if(MSVC)
    get_filename_component(PE_LOADER_MSVC_ARCH_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(PE_LOADER_MSVC_HOST_BIN "${PE_LOADER_MSVC_ARCH_BIN}" DIRECTORY)
    get_filename_component(PE_LOADER_MSVC_BIN "${PE_LOADER_MSVC_HOST_BIN}" DIRECTORY)
    get_filename_component(PE_LOADER_MSVC_TOOLSET "${PE_LOADER_MSVC_BIN}" DIRECTORY)
    set(PE_LOADER_STD_MODULE "${PE_LOADER_MSVC_TOOLSET}/modules/std.ixx")
    if(NOT EXISTS "${PE_LOADER_STD_MODULE}")
        message(FATAL_ERROR "MSVC std module was not found at ${PE_LOADER_STD_MODULE}")
    endif()
endif()

target_sources(pe_loader
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pe_loader.cppm
    PRIVATE
        src/pe_loader.cpp
)

if(MSVC)
    target_sources(pe_loader
        PUBLIC
            FILE_SET msvc_std_modules TYPE CXX_MODULES
            BASE_DIRS "${PE_LOADER_MSVC_TOOLSET}/modules"
            FILES "${PE_LOADER_STD_MODULE}"
    )
endif()
add_library(NewGhidra::PeLoader ALIAS pe_loader)

target_compile_features(pe_loader PUBLIC cxx_std_23)

if(MSVC)
    target_compile_options(pe_loader PRIVATE /W3)
else()
    target_compile_options(pe_loader PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(tests)
