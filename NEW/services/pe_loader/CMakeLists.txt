add_library(pe_loader STATIC)
target_sources(pe_loader PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pe_loader.cppm
)
add_library(NewGhidra::PeLoader ALIAS pe_loader)
target_compile_features(pe_loader PUBLIC cxx_std_23)
target_link_libraries(pe_loader PUBLIC NewGhidra::Std)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_library(new_ghidra_pe_loader_service STATIC)
target_sources(new_ghidra_pe_loader_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES pe_loader_service.cppm
)
add_library(NewGhidra::PeLoaderService ALIAS new_ghidra_pe_loader_service)
target_link_libraries(new_ghidra_pe_loader_service PUBLIC
    NewGhidra::Core NewGhidra::TranslationEngine pe_loader
)
target_compile_features(new_ghidra_pe_loader_service PUBLIC cxx_std_23)
