add_library(new_ghidra_sleigh_service STATIC)
target_sources(new_ghidra_sleigh_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES sleigh_service.cppm decoder_resource.cppm
)
add_library(NewGhidra::Sleigh ALIAS new_ghidra_sleigh_service)
target_link_libraries(new_ghidra_sleigh_service PUBLIC NewGhidra::Core NewGhidra::RuntimeWorkers NewGhidra::TranslationEngine NewGhidra::SleighRuntime)
target_compile_features(new_ghidra_sleigh_service PUBLIC cxx_std_23)
