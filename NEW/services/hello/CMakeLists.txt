add_library(hello_service STATIC)
target_sources(hello_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES src/hello.cppm
)
add_library(ReCode::Hello ALIAS hello_service)
target_link_libraries(hello_service PUBLIC ReCode::Core)
target_compile_features(hello_service PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
