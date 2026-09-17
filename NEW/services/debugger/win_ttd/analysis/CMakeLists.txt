add_library(recode_win_ttd_analysis STATIC)
target_sources(recode_win_ttd_analysis PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES function_call_analyzer.cppm)
add_library(ReCode::WinTtdAnalysis ALIAS recode_win_ttd_analysis)
target_link_libraries(recode_win_ttd_analysis PUBLIC ReCode::Core ReCode::Std)
target_compile_features(recode_win_ttd_analysis PUBLIC cxx_std_23)

if(RECODE_HAS_TTD_REPLAY)
    target_compile_definitions(recode_win_ttd_analysis PUBLIC RECODE_HAS_TTD_REPLAY)
    target_link_libraries(recode_win_ttd_analysis PRIVATE Microsoft.TimeTravelDebugging.Apis::TTDReplay)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
