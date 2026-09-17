add_library(new_ghidra_win_ttd_analysis STATIC)
target_sources(new_ghidra_win_ttd_analysis PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES function_call_analyzer.cppm)
add_library(NewGhidra::WinTtdAnalysis ALIAS new_ghidra_win_ttd_analysis)
target_link_libraries(new_ghidra_win_ttd_analysis PUBLIC NewGhidra::Core NewGhidra::Std)
target_compile_features(new_ghidra_win_ttd_analysis PUBLIC cxx_std_23)

if(NEW_GHIDRA_HAS_TTD_REPLAY)
    target_compile_definitions(new_ghidra_win_ttd_analysis PUBLIC NEW_GHIDRA_HAS_TTD_REPLAY)
    target_link_libraries(new_ghidra_win_ttd_analysis PRIVATE Microsoft.TimeTravelDebugging.Apis::TTDReplay)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
