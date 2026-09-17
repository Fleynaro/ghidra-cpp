find_package(GTest CONFIG REQUIRED)

if(MSVC)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/data/debugger_debuggee.exe"
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/build.bat"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/data/build.bat"
            "${CMAKE_CURRENT_SOURCE_DIR}/data/debugger_debuggee.cpp"
        COMMENT "Building the MSVC debugger integration debuggee"
    )
    add_custom_target(win_dbg_eng_debuggee
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/debugger_debuggee.exe"
    )
else()
    add_custom_target(win_dbg_eng_debuggee)
endif()

add_executable(win_dbg_eng_tests)
target_sources(win_dbg_eng_tests PRIVATE
    FILE_SET cxx_modules TYPE CXX_MODULES FILES debugger_contract_tests.cppm
)
add_dependencies(win_dbg_eng_tests win_dbg_eng_debuggee)
target_link_libraries(win_dbg_eng_tests PRIVATE
    ReCode::WinDbgEng
    ReCode::Std
    GTest::gtest_main
)
if(WIN32)
    target_include_directories(win_dbg_eng_tests PRIVATE
        "${DBGENG_SDK_VERSION_DIR}/um"
        "${DBGENG_SDK_VERSION_DIR}/shared"
        "${DBGENG_SDK_VERSION_DIR}/ucrt"
        "${DBGENG_MSVC_VERSION}/include"
    )
    target_link_directories(win_dbg_eng_tests PRIVATE
        "${DBGENG_SDK_LIB_VERSION_DIR}/um/x64"
        "${DBGENG_SDK_LIB_VERSION_DIR}/ucrt/x64"
        "${DBGENG_MSVC_VERSION}/lib/x64"
    )
endif()
target_compile_definitions(win_dbg_eng_tests PRIVATE
    DEBUGGER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
target_compile_features(win_dbg_eng_tests PRIVATE cxx_std_23)
add_test(NAME win_dbg_eng_tests COMMAND win_dbg_eng_tests)
set_tests_properties(win_dbg_eng_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
    TIMEOUT 60
    RUN_SERIAL TRUE
    RESOURCE_LOCK win_debugger_runtime
)
