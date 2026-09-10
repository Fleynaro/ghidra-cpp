cmake_minimum_required(VERSION 3.28)

project(NewGhidraCpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_subdirectory(features)

add_executable(new_ghidra_app src/main.cpp)
target_link_libraries(new_ghidra_app PRIVATE hello_feature)
