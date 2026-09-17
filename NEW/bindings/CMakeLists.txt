add_library(recode_cpp_bindings STATIC)
target_sources(recode_cpp_bindings PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        cpp/runtime.cppm
        cpp/project.cppm
        cpp/commands.cppm
        cpp/queries.cppm
        cpp/results.cppm
        cpp/debugger.cppm
)
add_library(ReCode::CppBindings ALIAS recode_cpp_bindings)
target_link_libraries(recode_cpp_bindings PUBLIC ReCode::RuntimeProject)
target_compile_features(recode_cpp_bindings PUBLIC cxx_std_23)
