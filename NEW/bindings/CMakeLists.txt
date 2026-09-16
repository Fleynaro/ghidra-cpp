add_library(new_ghidra_cpp_bindings STATIC)
target_sources(new_ghidra_cpp_bindings PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        cpp/runtime.cppm
        cpp/project.cppm
        cpp/commands.cppm
        cpp/queries.cppm
        cpp/results.cppm
        cpp/debugger.cppm
)
add_library(NewGhidra::CppBindings ALIAS new_ghidra_cpp_bindings)
target_link_libraries(new_ghidra_cpp_bindings PUBLIC NewGhidra::RuntimeProject)
target_compile_features(new_ghidra_cpp_bindings PUBLIC cxx_std_23)
