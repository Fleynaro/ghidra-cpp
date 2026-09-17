add_library(new_ghidra_ttd_recorder STATIC)
target_sources(new_ghidra_ttd_recorder PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES win_ttd.cppm
)
add_library(NewGhidra::TtdRecorder ALIAS new_ghidra_ttd_recorder)
target_link_libraries(new_ghidra_ttd_recorder PUBLIC NewGhidra::Core NewGhidra::Std)
target_compile_features(new_ghidra_ttd_recorder PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
