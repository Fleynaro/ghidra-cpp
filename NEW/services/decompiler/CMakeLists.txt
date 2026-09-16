find_package(pugixml CONFIG REQUIRED)

set(NEW_GHIDRA_DECOMPILER_INTERFACE_MODULE
    src/ghidra_decompiler.cppm
    src/block_switch.cppm
    src/grammar.cppm
    src/graph.cppm
    src/signature.cppm
    src/paramid.cppm
)

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

add_library(new_ghidra_decompiler STATIC)
add_library(NewGhidra::Decompiler ALIAS new_ghidra_decompiler)
set_target_properties(new_ghidra_decompiler PROPERTIES ARCHIVE_OUTPUT_NAME "new_ghidra_decompiler_engine")
if(MSVC)
    set_target_properties(new_ghidra_decompiler PROPERTIES COMPILE_PDB_NAME "new_ghidra_decompiler_engine")
endif()
target_sources(new_ghidra_decompiler
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
        FILES ${NEW_GHIDRA_DECOMPILER_INTERFACE_MODULE}
    PRIVATE ${NEW_GHIDRA_DECOMPILER_IMPLEMENTATION_MODULES}
)
target_link_libraries(new_ghidra_decompiler PRIVATE pugixml::pugixml PUBLIC NewGhidra::Std)
target_include_directories(new_ghidra_decompiler PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(new_ghidra_decompiler PUBLIC cxx_std_23)

add_library(new_ghidra_decompiler_frontend STATIC)
add_library(NewGhidra::DecompilerFrontend ALIAS new_ghidra_decompiler_frontend)
target_sources(new_ghidra_decompiler_frontend
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/decompiler.cppm
    PRIVATE src/decompiler_impl.cppm
)
target_include_directories(new_ghidra_decompiler_frontend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(new_ghidra_decompiler_frontend PUBLIC cxx_std_23)
target_link_libraries(new_ghidra_decompiler_frontend PUBLIC NewGhidra::Core NewGhidra::Decompiler NewGhidra::SleighRuntime
                                                            PRIVATE pugixml::pugixml)

add_executable(decompiler_cli cli/decompiler_cli.cppm)
set_target_properties(decompiler_cli PROPERTIES OUTPUT_NAME "new_ghidra_decompiler")
target_link_libraries(decompiler_cli PRIVATE NewGhidra::DecompilerFrontend)
target_compile_features(decompiler_cli PRIVATE cxx_std_23)

if(BUILD_TESTING)
    add_test(NAME decompiler_cli_help COMMAND decompiler_cli --help)
    set_tests_properties(decompiler_cli_help PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    add_subdirectory(tests)
endif()

add_library(new_ghidra_decompiler_service STATIC)
target_sources(new_ghidra_decompiler_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        decompiler_service.cppm
        decompiler_types.cppm
        provider_adapters.cppm
        decompilation_result.cppm
)
add_library(NewGhidra::DecompilerService ALIAS new_ghidra_decompiler_service)
target_link_libraries(new_ghidra_decompiler_service PUBLIC
    NewGhidra::Core NewGhidra::RuntimeWorkers NewGhidra::DecompilerFrontend
)
target_compile_features(new_ghidra_decompiler_service PUBLIC cxx_std_23)
