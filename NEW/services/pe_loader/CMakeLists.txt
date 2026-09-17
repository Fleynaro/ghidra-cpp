add_library(pe_loader STATIC)
target_sources(pe_loader PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pe_loader.cppm
)
add_library(ReCode::PeLoader ALIAS pe_loader)
target_compile_features(pe_loader PUBLIC cxx_std_23)
target_link_libraries(pe_loader PUBLIC ReCode::Std)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_library(recode_pe_loader_service STATIC)
target_sources(recode_pe_loader_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES pe_loader_service.cppm
)
add_library(ReCode::PeLoaderService ALIAS recode_pe_loader_service)
target_link_libraries(recode_pe_loader_service PUBLIC
    ReCode::Core ReCode::TranslationEngine pe_loader
)
target_compile_features(recode_pe_loader_service PUBLIC cxx_std_23)
