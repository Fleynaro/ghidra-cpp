add_library(pe_loader STATIC)

target_sources(pe_loader
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pe_loader.cppm
)

add_library(NewGhidra::PeLoader ALIAS pe_loader)

target_compile_features(pe_loader PUBLIC cxx_std_23)
target_link_libraries(pe_loader PUBLIC NewGhidra::Std)

if(MSVC)
    target_compile_options(pe_loader PRIVATE /W3)
else()
    target_compile_options(pe_loader PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
