add_library(pe_loader STATIC)
target_sources(pe_loader
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES pe_loader.cppm
    PRIVATE
        pe_loader.cpp
)
add_library(NewGhidra::PeLoader ALIAS pe_loader)

target_compile_features(pe_loader PUBLIC cxx_std_23)

if(MSVC)
    target_compile_options(pe_loader PRIVATE /W3)
else()
    target_compile_options(pe_loader PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(tests)
