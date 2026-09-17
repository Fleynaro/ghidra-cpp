option(NEW_GHIDRA_REQUIRE_TTD_REPLAY "Fail configuration unless real Microsoft TTD Replay is available" OFF)

if(WIN32)
    add_custom_target(new_ghidra_win_ttd_setup
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/setup_dependencies.bat"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        USES_TERMINAL
        COMMENT "Restoring Windows TTD API and Replay runtime dependencies"
    )
else()
    add_custom_target(new_ghidra_win_ttd_setup
        COMMAND ${CMAKE_COMMAND} -E echo "TTD dependency setup requires Windows"
        COMMAND ${CMAKE_COMMAND} -E false
        USES_TERMINAL
    )
endif()

add_library(new_ghidra_win_ttd_replay STATIC)
target_sources(new_ghidra_win_ttd_replay PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES win_ttd.cppm)
add_library(NewGhidra::WinTtdReplay ALIAS new_ghidra_win_ttd_replay)
target_link_libraries(new_ghidra_win_ttd_replay PUBLIC NewGhidra::Core NewGhidra::Std)
target_compile_features(new_ghidra_win_ttd_replay PUBLIC cxx_std_23)

if(WIN32)
    if(DEFINED ENV{TTD_RUNTIME_DIR} AND NOT "$ENV{TTD_RUNTIME_DIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{TTD_RUNTIME_DIR}" TTD_RUNTIME_DIR_ENV)
        set(TTD_RUNTIME_DIR "${TTD_RUNTIME_DIR_ENV}" CACHE PATH "Directory containing TTDReplay.dll and TTDReplayCPU.dll" FORCE)
    else()
        set(TTD_RUNTIME_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/runtime/x64")
        if(NOT TTD_RUNTIME_DIR AND EXISTS "${TTD_RUNTIME_DIR_DEFAULT}/TTDReplay.dll" AND EXISTS "${TTD_RUNTIME_DIR_DEFAULT}/TTDReplayCPU.dll")
            set(TTD_RUNTIME_DIR "${TTD_RUNTIME_DIR_DEFAULT}" CACHE PATH "Directory containing TTDReplay.dll and TTDReplayCPU.dll" FORCE)
        elseif(NOT DEFINED TTD_RUNTIME_DIR)
            set(TTD_RUNTIME_DIR "" CACHE PATH "Directory containing TTDReplay.dll and TTDReplayCPU.dll")
        endif()
    endif()
    if(DEFINED ENV{TTD_APIS_PACKAGE_DIR} AND NOT "$ENV{TTD_APIS_PACKAGE_DIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{TTD_APIS_PACKAGE_DIR}" TTD_APIS_PACKAGE_DIR_ENV)
        set(TTD_APIS_PACKAGE_DIR "${TTD_APIS_PACKAGE_DIR_ENV}" CACHE PATH "Directory containing Microsoft TTD API package" FORCE)
    elseif(NOT DEFINED TTD_APIS_PACKAGE_DIR OR TTD_APIS_PACKAGE_DIR STREQUAL "")
        set(TTD_APIS_PACKAGE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Microsoft.TimeTravelDebugging.Apis.0.9.5"
            CACHE PATH "Directory containing Microsoft TTD API package")
    endif()
    file(TO_CMAKE_PATH "${TTD_APIS_PACKAGE_DIR}" TTD_APIS_PACKAGE_DIR_CMAKE)
    find_package(Microsoft.TimeTravelDebugging.Apis CONFIG QUIET PATHS "${TTD_APIS_PACKAGE_DIR_CMAKE}/CMake" NO_DEFAULT_PATH)
    if(Microsoft.TimeTravelDebugging.Apis_FOUND)
        set(NEW_GHIDRA_HAS_TTD_REPLAY ON)
        target_compile_definitions(new_ghidra_win_ttd_replay PUBLIC NEW_GHIDRA_HAS_TTD_REPLAY)
        target_link_libraries(new_ghidra_win_ttd_replay PRIVATE Microsoft.TimeTravelDebugging.Apis::TTDReplay)
        if(TTD_RUNTIME_DIR AND EXISTS "${TTD_RUNTIME_DIR}/TTDReplay.dll" AND EXISTS "${TTD_RUNTIME_DIR}/TTDReplayCPU.dll")
            add_custom_command(TARGET new_ghidra_win_ttd_replay POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TTD_RUNTIME_DIR}/TTDReplay.dll" "$<TARGET_FILE_DIR:new_ghidra_win_ttd_replay>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TTD_RUNTIME_DIR}/TTDReplayCPU.dll" "$<TARGET_FILE_DIR:new_ghidra_win_ttd_replay>"
            )
        elseif(NEW_GHIDRA_REQUIRE_TTD_REPLAY)
            message(FATAL_ERROR "Real TTD Replay was requested, but TTDReplay.dll and TTDReplayCPU.dll were not found. Set TTD_RUNTIME_DIR or run win_ttd/setup_dependencies.bat.")
        endif()
    else()
        if(NEW_GHIDRA_REQUIRE_TTD_REPLAY)
            message(FATAL_ERROR "Real TTD Replay was requested, but Microsoft.TimeTravelDebugging.Apis was not found. Run win_ttd/setup_dependencies.bat or set TTD_APIS_PACKAGE_DIR.")
        endif()
        message(WARNING "Microsoft TTD Replay API package not found; WinTtd service will report unsupported")
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_subdirectory(recorder)
