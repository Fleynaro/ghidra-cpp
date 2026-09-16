cmake_minimum_required(VERSION 3.28)

project(NewGhidraCpp VERSION 0.1.0 LANGUAGES CXX)

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

add_executable(new_ghidra_app src/main.cpp)
target_link_libraries(new_ghidra_app PRIVATE
    hello_service
    NewGhidra::DecompilerFrontend
    NewGhidra::Analyzer
)

if(BUILD_TESTING)
    # Keep the application smoke case in the normal build/test workflow.
    add_test(NAME new_ghidra_app_smoke COMMAND new_ghidra_app)
endif()
