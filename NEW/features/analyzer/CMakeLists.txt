add_library(analyzer STATIC)

set(ANALYZER_IMPLEMENTATION_MODULES
    disassemble_entry_points/disassemble_entry_points.cppm
    function_start_search/function_start_search.cppm
    subroutine_references/subroutine_references.cppm
    function_body/function_body.cppm
    reference/reference.cppm
    data_reference/data_reference.cppm
    scalar_operand_references/scalar_operand_references.cppm
    stack/stack.cppm
    constant_propagation/constant_propagation.cppm
    non_returning_functions/non_returning_functions.cppm
)
list(TRANSFORM ANALYZER_IMPLEMENTATION_MODULES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

target_sources(analyzer
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES src/analyzer.cppm
    PRIVATE
        src/analyzer.cpp
        ${ANALYZER_IMPLEMENTATION_MODULES}
)

add_library(NewGhidra::Analyzer ALIAS analyzer)
target_compile_features(analyzer PUBLIC cxx_std_23)
target_link_libraries(analyzer PUBLIC NewGhidra::PeLoader NewGhidra::SleighRuntime)

if(MSVC)
    target_compile_options(analyzer PRIVATE /W3)
else()
    target_compile_options(analyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
