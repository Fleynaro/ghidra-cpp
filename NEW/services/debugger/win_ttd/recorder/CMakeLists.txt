add_library(recode_ttd_recorder STATIC)
target_sources(recode_ttd_recorder PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES win_ttd.cppm
)
add_library(ReCode::TtdRecorder ALIAS recode_ttd_recorder)
target_link_libraries(recode_ttd_recorder PUBLIC ReCode::Core ReCode::Std)
target_compile_features(recode_ttd_recorder PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
