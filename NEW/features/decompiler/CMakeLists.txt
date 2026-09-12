cmake_minimum_required(VERSION 3.28)

project(NewGhidraDecompiler LANGUAGES CXX)

find_package(pugixml CONFIG REQUIRED)

# The former header declarations are exported from one aggregate interface.
# The restored named interfaces keep the original public API boundaries for
# grammar parsing, Renoir graph export, signatures, and parameter measures.
set(NEW_GHIDRA_DECOMPILER_INTERFACE_MODULE
    src/ghidra_decompiler.cppm
    src/block_switch.cppm
    src/grammar.cppm
    src/graph.cppm
    src/signature.cppm
    src/paramid.cppm
)

# This explicit list mirrors the migrated implementation units. It prevents
# dormant or generated source files from silently entering the module target.
set(NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES
    src/action.cppm
    src/address.cppm
    src/architecture.cppm
    src/bitfield.cppm
    src/block.cppm
    src/blockaction.cppm
    src/capability.cppm
    src/cast.cppm
    src/comment.cppm
    src/condexe.cppm
    src/constseq.cppm
    src/coreaction.cppm
    src/cover.cppm
    src/cpool.cppm
    src/crc32.cppm
    src/database.cppm
    src/double.cppm
    src/dynamic.cppm
    src/emulate.cppm
    src/emulateutil.cppm
    src/expression.cppm
    src/float.cppm
    src/flow.cppm
    src/fspec.cppm
    src/funcdata.cppm
    src/funcdata_block.cppm
    src/funcdata_op.cppm
    src/funcdata_varnode.cppm
    src/globalcontext.cppm
    src/heritage.cppm
    src/jumptable.cppm
    src/loadimage.cppm
    src/marshal.cppm
    src/memstate.cppm
    src/merge.cppm
    src/modelrules.cppm
    src/multiprecision.cppm
    src/op.cppm
    src/opbehavior.cppm
    src/opcodes.cppm
    src/options.cppm
    src/override.cppm
    src/pcodeinject.cppm
    src/pcoderaw.cppm
    src/prefersplit.cppm
    src/prettyprint.cppm
    src/printc.cppm
    src/printjava.cppm
    src/printlanguage.cppm
    src/rangeutil.cppm
    src/ruleaction.cppm
    src/space.cppm
    src/stringmanage.cppm
    src/subflow.cppm
    src/transform.cppm
    src/translate.cppm
    src/type.cppm
    src/typeop.cppm
    src/unionresolve.cppm
    src/userop.cppm
    src/variable.cppm
    src/varmap.cppm
    src/varnode.cppm
    src/xml.cppm
)
list(TRANSFORM NEW_GHIDRA_DECOMPILER_INTERFACE_MODULE
    PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/"
)
list(TRANSFORM NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES
    PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/"
)

add_library(new_ghidra_decompiler STATIC)
add_library(NewGhidra::Decompiler ALIAS new_ghidra_decompiler)

target_sources(new_ghidra_decompiler
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES
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
            FILE_SET cxx_modules TYPE CXX_MODULES FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/decompiler.cppm"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/decompiler_impl.cppm"
    )
    add_library(NewGhidra::DecompilerFrontend ALIAS new_ghidra_decompiler_frontend)

    target_include_directories(new_ghidra_decompiler_frontend PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_compile_features(new_ghidra_decompiler_frontend PUBLIC cxx_std_23)
    target_link_libraries(new_ghidra_decompiler_frontend
        PUBLIC NewGhidra::Decompiler
        PUBLIC NewGhidra::SleighRuntime
    )

    if(MSVC)
        target_compile_options(new_ghidra_decompiler_frontend PRIVATE /W3)
    else()
        target_compile_options(new_ghidra_decompiler_frontend PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_subdirectory(tests)
endif()
