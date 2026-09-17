cmake_minimum_required(VERSION 3.28)

project(ReCode VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

add_subdirectory(core)
add_subdirectory(runtime)
add_subdirectory(services)
add_subdirectory(runtime/project)
add_subdirectory(tests)
add_subdirectory(bindings)

add_executable(recode_app src/main.cpp)
target_link_libraries(recode_app PRIVATE
    hello_service
    ReCode::DecompilerFrontend
    ReCode::Analyzer
)

if(BUILD_TESTING)
    # Keep the application smoke case in the normal build/test workflow.
    add_test(NAME recode_app_smoke COMMAND recode_app)
endif()
