add_subdirectory(translation_engine)
add_subdirectory(hello)
add_subdirectory(pe_loader)
add_subdirectory(sleigh)
add_subdirectory(function_id)
add_subdirectory(decompiler)
add_subdirectory(analyzers)
add_subdirectory(debugger)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_custom_target(recode_services)
add_dependencies(recode_services
    hello_service
    pe_loader
    sleigh_runtime
    function_id
    recode_decompiler
    recode_decompiler_frontend
    analyzer
    recode_analyzer_services
    recode_win_dbg_eng
    recode_win_ttd_replay
    recode_ttd_recorder
    recode_win_ttd_analysis
)
