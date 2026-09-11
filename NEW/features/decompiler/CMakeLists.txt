cmake_minimum_required(VERSION 3.28)

project(NewGhidraDecompiler LANGUAGES CXX)

if(MSVC)
    get_filename_component(DEC_MSVC_ARCH_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(DEC_MSVC_HOST_BIN "${DEC_MSVC_ARCH_BIN}" DIRECTORY)
    get_filename_component(DEC_MSVC_BIN "${DEC_MSVC_HOST_BIN}" DIRECTORY)
    get_filename_component(DEC_MSVC_TOOLSET "${DEC_MSVC_BIN}" DIRECTORY)
    set(DEC_STD_MODULE "${DEC_MSVC_TOOLSET}/modules/std.ixx")
    if(NOT EXISTS "${DEC_STD_MODULE}")
        message(FATAL_ERROR "MSVC std module was not found at ${DEC_STD_MODULE}")
    endif()
endif()

# This list mirrors the original native decompiler Makefile's CORE and
# DECCORE partitions, plus the portable EXTRA implementations required by
# those partitions.  Sleigh, Ghidra-host, interface, test, and loader sources
# are intentionally not part of this first standalone target.
set(NEW_GHIDRA_DECOMPILER_SOURCES
    src/xml.cc
    src/marshal.cc
    src/space.cc
    src/float.cc
    src/address.cc
    src/pcoderaw.cc
    src/translate.cc
    src/opcodes.cc
    src/globalcontext.cc
    src/capability.cc
    src/architecture.cc
    src/options.cc
    src/graph.cc
    src/cover.cc
    src/block.cc
    src/cast.cc
    src/typeop.cc
    src/database.cc
    src/cpool.cc
    src/comment.cc
    src/stringmanage.cc
    src/modelrules.cc
    src/fspec.cc
    src/action.cc
    src/loadimage.cc
    src/varnode.cc
    src/op.cc
    src/type.cc
    src/variable.cc
    src/varmap.cc
    src/jumptable.cc
    src/emulate.cc
    src/emulateutil.cc
    src/flow.cc
    src/userop.cc
    src/expression.cc
    src/multiprecision.cc
    src/funcdata.cc
    src/funcdata_block.cc
    src/funcdata_op.cc
    src/funcdata_varnode.cc
    src/unionresolve.cc
    src/pcodeinject.cc
    src/heritage.cc
    src/prefersplit.cc
    src/rangeutil.cc
    src/ruleaction.cc
    src/subflow.cc
    src/blockaction.cc
    src/merge.cc
    src/double.cc
    src/transform.cc
    src/constseq.cc
    src/bitfield.cc
    src/coreaction.cc
    src/condexe.cc
    src/override.cc
    src/dynamic.cc
    src/crc32.cc
    src/prettyprint.cc
    src/printlanguage.cc
    src/printc.cc
    src/printjava.cc
    src/memstate.cc
    src/opbehavior.cc
    src/paramid.cc
    src/signature.cc
)

add_library(new_ghidra_decompiler STATIC ${NEW_GHIDRA_DECOMPILER_SOURCES})
add_library(NewGhidra::Decompiler ALIAS new_ghidra_decompiler)

target_include_directories(new_ghidra_decompiler
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Keep the legacy engine buildable with the C++11 language level used by the
# original Makefile, even when this module is later added to a newer parent.
target_compile_features(new_ghidra_decompiler PUBLIC cxx_std_11)
set_target_properties(new_ghidra_decompiler PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(new_ghidra_decompiler PRIVATE /W3)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # The original C++11 source uses a legacy all-ones signed literal in a
    # uintb initializer; Clang diagnoses that preserved initializer as a hard
    # narrowing error under strict C++11 mode.
    target_compile_options(new_ghidra_decompiler PRIVATE
        -Wno-c++11-narrowing
        -Wall
        -Wextra
        -Wno-sign-compare
    )
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
            src/decompiler.cpp
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
