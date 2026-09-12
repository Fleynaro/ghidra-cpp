cmake_minimum_required(VERSION 3.28)

project(NewGhidraDecompiler LANGUAGES CXX)

find_package(pugixml CONFIG REQUIRED)

# The former header declarations are exported from one aggregate interface.
# MSVC 14.51 loses complete declarations from the mutually dependent block
# partition when it is imported by another partition, so the aggregate is the
# documented fallback for the coupled engine. The graph-verified
# parser/compiler sources remain outside this target because the active engine
# is built without CPUI_RULECOMPILE and uses external XML parsing.
set(NEW_GHIDRA_DECOMPILER_INTERFACE_MODULE
    src/ghidra_decompiler.cppm
    src/block_switch.cppm
)

file(GLOB NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
)
set(NEW_GHIDRA_DECOMPILER_NON_IMPLEMENTATION_NAMES
    ghidra_decompiler.cppm
    block_switch.cppm
    decompiler.cppm
    decompiler_impl.cppm
)
set(NEW_GHIDRA_DECOMPILER_ACTIVE_IMPLEMENTATIONS)
foreach(NEW_GHIDRA_DECOMPILER_MODULE IN LISTS NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES)
    get_filename_component(NEW_GHIDRA_DECOMPILER_MODULE_NAME
        "${NEW_GHIDRA_DECOMPILER_MODULE}" NAME
    )
    if(NOT NEW_GHIDRA_DECOMPILER_MODULE_NAME MATCHES "_interface\\.cppm$"
       AND NOT NEW_GHIDRA_DECOMPILER_MODULE_NAME IN_LIST NEW_GHIDRA_DECOMPILER_NON_IMPLEMENTATION_NAMES)
        list(APPEND NEW_GHIDRA_DECOMPILER_ACTIVE_IMPLEMENTATIONS
            "${NEW_GHIDRA_DECOMPILER_MODULE}"
        )
    endif()
endforeach()
set(NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES
    ${NEW_GHIDRA_DECOMPILER_ACTIVE_IMPLEMENTATIONS}
)

add_library(new_ghidra_decompiler STATIC)
add_library(NewGhidra::Decompiler ALIAS new_ghidra_decompiler)

target_sources(new_ghidra_decompiler
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES BASE_DIRS src FILES
            ${NEW_GHIDRA_DECOMPILER_INTERFACE_MODULE}
    PRIVATE
        ${NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES}
)

target_link_libraries(new_ghidra_decompiler PRIVATE pugixml::pugixml)
if(TARGET NewGhidra::Std)
    target_link_libraries(new_ghidra_decompiler PUBLIC NewGhidra::Std)
elseif(MSVC)
    get_filename_component(DEC_MSVC_ARCH_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(DEC_MSVC_HOST_BIN "${DEC_MSVC_ARCH_BIN}" DIRECTORY)
    get_filename_component(DEC_MSVC_BIN "${DEC_MSVC_HOST_BIN}" DIRECTORY)
    get_filename_component(DEC_MSVC_TOOLSET "${DEC_MSVC_BIN}" DIRECTORY)
    set(DEC_STD_MODULE "${DEC_MSVC_TOOLSET}/modules/std.ixx")
    if(NOT EXISTS "${DEC_STD_MODULE}")
        message(FATAL_ERROR "MSVC std module was not found at ${DEC_STD_MODULE}")
    endif()
    target_sources(new_ghidra_decompiler
        PUBLIC
            FILE_SET msvc_std_modules TYPE CXX_MODULES BASE_DIRS "${DEC_MSVC_TOOLSET}"
                FILES "${DEC_STD_MODULE}"
    )
endif()
target_include_directories(new_ghidra_decompiler PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_compile_features(new_ghidra_decompiler PUBLIC cxx_std_23)
set_target_properties(new_ghidra_decompiler PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(new_ghidra_decompiler PRIVATE /W3)
else()
    target_compile_options(new_ghidra_decompiler PRIVATE -Wall -Wextra -Wno-sign-compare)
endif()

if(TARGET NewGhidra::SleighRuntime)
    find_package(GTest CONFIG REQUIRED)
    add_library(new_ghidra_decompiler_frontend STATIC)
    target_sources(new_ghidra_decompiler_frontend
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES src/decompiler.cppm
        PRIVATE
            src/decompiler_impl.cppm
    )
    add_library(NewGhidra::DecompilerFrontend ALIAS new_ghidra_decompiler_frontend)

    target_include_directories(new_ghidra_decompiler_frontend PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_compile_features(new_ghidra_decompiler_frontend PUBLIC cxx_std_23)
    target_link_libraries(new_ghidra_decompiler_frontend
        PUBLIC NewGhidra::Decompiler
        PRIVATE NewGhidra::SleighRuntime
    )

    if(MSVC)
        target_compile_options(new_ghidra_decompiler_frontend PRIVATE /W3)
    else()
        target_compile_options(new_ghidra_decompiler_frontend PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_subdirectory(tests)
endif()
