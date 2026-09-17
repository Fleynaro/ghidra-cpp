set(RECODE_CORE_MODULES
    core.cppm
    domain/domain.cppm
    domain/identifiers.cppm
    domain/diagnostics.cppm
    domain/bytes.cppm
    domain/address_space.cppm
    domain/address.cppm
    domain/address_range.cppm
    domain/address_factory.cppm
    domain/binary.cppm
    domain/relocation.cppm
    domain/storage_location.cppm
    domain/register.cppm
    domain/scalar.cppm
    domain/pcode_opcode.cppm
    domain/pcode.cppm
    domain/operand.cppm
    domain/flow.cppm
    domain/instruction.cppm
    domain/instruction_reference.cppm
    domain/basic_block.cppm
    domain/function_signature.cppm
    domain/function.cppm
    domain/memory_region.cppm
    domain/symbol.cppm
    domain/reference.cppm
    domain/data_type.cppm
    domain/data_object.cppm
    domain/analysis_fact.cppm
    domain/architecture.cppm
    domain/processor_context.cppm
    domain/decoded_instruction.cppm
    domain/variable.cppm
    domain/function_id.cppm
    domain/decompilation.cppm
    domain/debugger.cppm
    domain/replay.cppm
    domain/trace_recording.cppm
    domain/trace_analysis.cppm
    contracts/contracts.cppm
    contracts/operation.cppm
    contracts/memory_provider.cppm
    contracts/architecture_provider.cppm
    contracts/project_query.cppm
    contracts/pcode_decoder.cppm
    contracts/pe_loader.cppm
    contracts/decompiler.cppm
    contracts/function_id.cppm
    contracts/function_id_database.cppm
    contracts/analyzer.cppm
    contracts/command.cppm
    contracts/event_store.cppm
    contracts/projection.cppm
    contracts/event_bus.cppm
    contracts/resource_manager.cppm
    contracts/debugger.cppm
    contracts/trace_recorder.cppm
    contracts/trace_analysis.cppm
    events/events.cppm
    events/event.cppm
    events/project_events.cppm
    events/memory_events.cppm
    events/code_events.cppm
    events/function_events.cppm
    events/symbol_events.cppm
    events/type_events.cppm
    events/analysis_events.cppm
)

if(MSVC)
    get_filename_component(CORE_MSVC_ARCH_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(CORE_MSVC_HOST_BIN "${CORE_MSVC_ARCH_BIN}" DIRECTORY)
    get_filename_component(CORE_MSVC_BIN "${CORE_MSVC_HOST_BIN}" DIRECTORY)
    get_filename_component(CORE_MSVC_TOOLSET "${CORE_MSVC_BIN}" DIRECTORY)
    set(CORE_STD_MODULE "${CORE_MSVC_TOOLSET}/modules/std.ixx")
    if(NOT EXISTS "${CORE_STD_MODULE}")
        message(FATAL_ERROR "MSVC std module was not found at ${CORE_STD_MODULE}")
    endif()
endif()

add_library(recode_std STATIC)
if(MSVC)
    target_sources(recode_std
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS "${CORE_MSVC_TOOLSET}/modules"
            FILES "${CORE_STD_MODULE}"
    )
endif()
add_library(ReCode::Std ALIAS recode_std)
target_compile_features(recode_std PUBLIC cxx_std_23)

add_library(recode_core STATIC)
target_sources(recode_core
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${RECODE_CORE_MODULES}
)
add_library(ReCode::Core ALIAS recode_core)
target_compile_features(recode_core PUBLIC cxx_std_23)
target_link_libraries(recode_core PUBLIC ReCode::Std)

if(MSVC)
    target_compile_options(recode_core PRIVATE /W3)
else()
    target_compile_options(recode_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
