add_library(analyzer_shared STATIC)
add_library(ReCode::AnalyzerShared ALIAS analyzer_shared)
target_compile_features(analyzer_shared PUBLIC cxx_std_23)
target_link_libraries(analyzer_shared PUBLIC
    ReCode::Core
    ReCode::PeLoader
    ReCode::SleighRuntime
    ReCode::DecompilerFrontend
)

add_library(analyzer STATIC analyzer_builtin.cpp)
add_library(ReCode::Analyzer ALIAS analyzer)
target_compile_features(analyzer PUBLIC cxx_std_23)
target_link_libraries(analyzer PUBLIC analyzer_shared)

if(MSVC)
    target_compile_options(analyzer_shared PRIVATE /W3)
else()
    target_compile_options(analyzer_shared PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(shared)
add_subdirectory(tests)

set(ANALYZER_NAMES
    aggressive_instruction_finder
    apply_data_archives
    ascii_strings
    call_convention_id
    call_fixup_installer
    condense_filler_bytes
    constant_propagation
    create_address_tables
    data_reference
    decompiler_parameter_id
    decompiler_switch_analysis
    demangler_microsoft
    disassemble_entry_points
    embedded_media
    external_entry_references
    function_id
    function_start_search
    non_returning_functions
    non_returning_functions_discovered
    non_returning_functions_known
    pdb_msdia
    pdb_universal
    reference
    scalar_operand_references
    shared_return_calls
    stack
    subroutine_references
    variadic_function_signature_override
    windows_pe_x86_propagate_external_parameters
    windows_resource_reference
    x86_constant_reference
)
foreach(analyzer_name IN LISTS ANALYZER_NAMES)
    add_subdirectory(${analyzer_name})
endforeach()

foreach(analyzer_name IN LISTS ANALYZER_NAMES)
    if(TARGET analyzer_${analyzer_name})
        target_link_libraries(analyzer PUBLIC analyzer_${analyzer_name})
    endif()
endforeach()

add_library(recode_analyzer_services STATIC)
target_sources(recode_analyzer_services PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES entry_materialization.cppm
)
add_library(ReCode::AnalyzerServices ALIAS recode_analyzer_services)
target_link_libraries(recode_analyzer_services PUBLIC ReCode::Core)
target_compile_features(recode_analyzer_services PUBLIC cxx_std_23)
