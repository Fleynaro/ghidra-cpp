add_subdirectory(translation_engine)
add_subdirectory(hello)
add_subdirectory(pe_loader)
add_subdirectory(sleigh)
add_subdirectory(function_id)
add_subdirectory(decompiler)
add_subdirectory(analyzers)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_custom_target(new_ghidra_services)
add_dependencies(new_ghidra_services
    hello_service
    pe_loader
    sleigh_runtime
    function_id
    new_ghidra_decompiler
    new_ghidra_decompiler_frontend
    analyzer
    new_ghidra_analyzer_services
)
