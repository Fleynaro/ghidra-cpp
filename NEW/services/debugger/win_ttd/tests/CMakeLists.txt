find_package(GTest CONFIG REQUIRED)

add_executable(win_ttd_tests)
target_sources(win_ttd_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES replay_contract_tests.cppm)
target_link_libraries(win_ttd_tests PRIVATE ReCode::WinTtdReplay ReCode::Std GTest::gtest_main)
target_compile_features(win_ttd_tests PRIVATE cxx_std_23)
if(RECODE_HAS_TTD_REPLAY)
    target_compile_definitions(win_ttd_tests PRIVATE RECODE_HAS_TTD_REPLAY)
endif()
if(WIN32 AND TTD_RUNTIME_DIR AND EXISTS "${TTD_RUNTIME_DIR}/TTDReplay.dll" AND EXISTS "${TTD_RUNTIME_DIR}/TTDReplayCPU.dll")
    add_custom_command(TARGET win_ttd_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TTD_RUNTIME_DIR}/TTDReplay.dll" "$<TARGET_FILE_DIR:win_ttd_tests>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TTD_RUNTIME_DIR}/TTDReplayCPU.dll" "$<TARGET_FILE_DIR:win_ttd_tests>"
    )
endif()
add_test(NAME win_ttd_tests COMMAND win_ttd_tests)
set_tests_properties(win_ttd_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.." TIMEOUT 120 RUN_SERIAL TRUE RESOURCE_LOCK win_debugger_runtime)
