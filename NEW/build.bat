@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

set "MODE=%~1"
if not defined MODE set "MODE=all"

if /I "%MODE%"=="help" goto usage
if /I "%MODE%"=="/?" goto usage
if /I "%MODE%"=="-h" goto usage

set "BUILD_DIR=%SCRIPT_DIR%build"
set "BUILD_TARGET=new_ghidra_app"
set "TEST_FILTER=^new_ghidra_app_smoke$"
set "RUN_TESTS=1"
set "FULL_BUILD=0"

if /I "%MODE%"=="all" (
    set "FULL_BUILD=1"
    set "BUILD_TARGET="
    set "TEST_FILTER="
    goto mode_selected
)
if /I "%MODE%"=="app" goto mode_selected
if /I "%MODE%"=="hello" (
    set "BUILD_TARGET=hello_feature_tests"
    set "TEST_FILTER=^hello_feature_tests$"
    goto mode_selected
)
if /I "%MODE%"=="sleigh" (
    set "BUILD_TARGET=sleigh_runtime_tests"
    set "TEST_FILTER=^sleigh_runtime_tests$"
    goto mode_selected
)
if /I "%MODE%"=="pe" (
    set "BUILD_TARGET=pe_loader_tests"
    set "TEST_FILTER=^pe_loader_tests$"
    goto mode_selected
)
if /I "%MODE%"=="function_id" (
    set "BUILD_TARGET=function_id_tests"
    set "TEST_FILTER=^function_id_tests$"
    goto mode_selected
)
if /I "%MODE%"=="decompiler" (
    set "BUILD_TARGET=decompiler_tests native_paramstore_tests native_circlerange_tests native_funcproto_tests decompiler_architecture_tests metadata_provider_tests decompiler_cli"
    set "TEST_FILTER=^(decompiler_tests|native_paramstore_tests|native_circlerange_tests|native_funcproto_tests|decompiler_architecture_tests|metadata_provider_tests|decompiler_cli_help)$"
    goto mode_selected
)
if /I "%MODE%"=="constant_propagation" (
    set "BUILD_TARGET=analyzer_constant_propagation analyzer_constant_propagation_tests"
    set "TEST_FILTER=^analyzer_constant_propagation_tests$"
    goto mode_selected
)
if /I "%MODE%"=="data_reference" (
    set "BUILD_TARGET=analyzer_data_reference analyzer_data_reference_tests"
    set "TEST_FILTER=^analyzer_data_reference_tests$"
    goto mode_selected
)
if /I "%MODE%"=="disassemble_entry_points" (
    set "BUILD_TARGET=analyzer_disassemble_entry_points analyzer_disassemble_entry_points_tests"
    set "TEST_FILTER=^analyzer_disassemble_entry_points_tests$"
    goto mode_selected
)
if /I "%MODE%"=="shared_function_body" (
    set "BUILD_TARGET=analyzer_shared_function_body_tests"
    set "TEST_FILTER=^analyzer_shared_function_body_tests$"
    goto mode_selected
)
if /I "%MODE%"=="function_start_search" (
    set "BUILD_TARGET=analyzer_function_start_search analyzer_function_start_search_tests"
    set "TEST_FILTER=^analyzer_function_start_search_tests$"
    goto mode_selected
)
if /I "%MODE%"=="non_returning_functions" (
    set "BUILD_TARGET=analyzer_non_returning_functions analyzer_non_returning_functions_tests"
    set "TEST_FILTER=^analyzer_non_returning_functions_tests$"
    goto mode_selected
)
if /I "%MODE%"=="reference" (
    set "BUILD_TARGET=analyzer_reference analyzer_reference_tests"
    set "TEST_FILTER=^analyzer_reference_tests$"
    goto mode_selected
)
if /I "%MODE%"=="scalar_operand_references" (
    set "BUILD_TARGET=analyzer_scalar_operand_references analyzer_scalar_operand_references_tests"
    set "TEST_FILTER=^analyzer_scalar_operand_references_tests$"
    goto mode_selected
)
if /I "%MODE%"=="stack" (
    set "BUILD_TARGET=analyzer_stack analyzer_stack_tests"
    set "TEST_FILTER=^analyzer_stack_tests$"
    goto mode_selected
)
if /I "%MODE%"=="subroutine_references" (
    set "BUILD_TARGET=analyzer_subroutine_references analyzer_subroutine_references_tests"
    set "TEST_FILTER=^analyzer_subroutine_references_tests$"
    goto mode_selected
)
if /I "%MODE%"=="aggressive_instruction_finder" (
    set "BUILD_TARGET=analyzer_aggressive_instruction_finder analyzer_aggressive_instruction_finder_tests"
    set "TEST_FILTER=^analyzer_aggressive_instruction_finder_tests$"
    goto mode_selected
)
if /I "%MODE%"=="apply_data_archives" (
    set "BUILD_TARGET=analyzer_apply_data_archives analyzer_apply_data_archives_tests"
    set "TEST_FILTER=^analyzer_apply_data_archives_tests$"
    goto mode_selected
)
if /I "%MODE%"=="ascii_strings" (
    set "BUILD_TARGET=analyzer_ascii_strings analyzer_ascii_strings_tests"
    set "TEST_FILTER=^analyzer_ascii_strings_tests$"
    goto mode_selected
)
if /I "%MODE%"=="call_convention_id" (
    set "BUILD_TARGET=analyzer_call_convention_id analyzer_call_convention_id_tests"
    set "TEST_FILTER=^analyzer_call_convention_id_tests$"
    goto mode_selected
)
if /I "%MODE%"=="call_fixup_installer" (
    set "BUILD_TARGET=analyzer_call_fixup_installer analyzer_call_fixup_installer_tests"
    set "TEST_FILTER=^analyzer_call_fixup_installer_tests$"
    goto mode_selected
)
if /I "%MODE%"=="condense_filler_bytes" (
    set "BUILD_TARGET=analyzer_condense_filler_bytes analyzer_condense_filler_bytes_tests"
    set "TEST_FILTER=^analyzer_condense_filler_bytes_tests$"
    goto mode_selected
)
if /I "%MODE%"=="create_address_tables" (
    set "BUILD_TARGET=analyzer_create_address_tables analyzer_create_address_tables_tests"
    set "TEST_FILTER=^analyzer_create_address_tables_tests$"
    goto mode_selected
)
if /I "%MODE%"=="decompiler_parameter_id" (
    set "BUILD_TARGET=analyzer_decompiler_parameter_id analyzer_decompiler_parameter_id_tests"
    set "TEST_FILTER=^analyzer_decompiler_parameter_id_tests$"
    goto mode_selected
)
if /I "%MODE%"=="decompiler_switch_analysis" (
    set "BUILD_TARGET=analyzer_decompiler_switch_analysis analyzer_decompiler_switch_analysis_tests"
    set "TEST_FILTER=^analyzer_decompiler_switch_analysis_tests$"
    goto mode_selected
)
if /I "%MODE%"=="demangler_microsoft" (
    set "BUILD_TARGET=analyzer_demangler_microsoft analyzer_demangler_microsoft_tests"
    set "TEST_FILTER=^analyzer_demangler_microsoft_tests$"
    goto mode_selected
)
if /I "%MODE%"=="embedded_media" (
    set "BUILD_TARGET=analyzer_embedded_media analyzer_embedded_media_tests"
    set "TEST_FILTER=^analyzer_embedded_media_tests$"
    goto mode_selected
)
if /I "%MODE%"=="external_entry_references" (
    set "BUILD_TARGET=analyzer_external_entry_references analyzer_external_entry_references_tests"
    set "TEST_FILTER=^analyzer_external_entry_references_tests$"
    goto mode_selected
)
if /I "%MODE%"=="function_id_analyzer" (
    set "BUILD_TARGET=analyzer_function_id analyzer_function_id_tests"
    set "TEST_FILTER=^analyzer_function_id_tests$"
    goto mode_selected
)
if /I "%MODE%"=="pdb_msdia" (
    set "BUILD_TARGET=analyzer_pdb_msdia analyzer_pdb_msdia_tests"
    set "TEST_FILTER=^analyzer_pdb_msdia_tests$"
    goto mode_selected
)
if /I "%MODE%"=="pdb_universal" (
    set "BUILD_TARGET=analyzer_pdb_universal analyzer_pdb_universal_tests"
    set "TEST_FILTER=^analyzer_pdb_universal_tests$"
    goto mode_selected
)
if /I "%MODE%"=="shared_return_calls" (
    set "BUILD_TARGET=analyzer_shared_return_calls analyzer_shared_return_calls_tests"
    set "TEST_FILTER=^analyzer_shared_return_calls_tests$"
    goto mode_selected
)
if /I "%MODE%"=="variadic_function_signature_override" (
    set "BUILD_TARGET=analyzer_variadic analyzer_variadic_tests"
    set "TEST_FILTER=^analyzer_variadic_function_signature_override_tests$"
    goto mode_selected
)
if /I "%MODE%"=="windows_pe_x86_propagate_external_parameters" (
    set "BUILD_TARGET=analyzer_winparams analyzer_winparams_tests"
    set "TEST_FILTER=^analyzer_windows_pe_x86_propagate_external_parameters_tests$"
    goto mode_selected
)
if /I "%MODE%"=="windows_resource_reference" (
    set "BUILD_TARGET=analyzer_windows_resource_reference analyzer_windows_resource_reference_tests"
    set "TEST_FILTER=^analyzer_windows_resource_reference_tests$"
    goto mode_selected
)
if /I "%MODE%"=="x86_constant_reference" (
    set "BUILD_TARGET=analyzer_x86_constant_reference analyzer_x86_constant_reference_tests"
    set "TEST_FILTER=^analyzer_x86_constant_reference_tests$"
    goto mode_selected
)
if /I "%MODE%"=="analyzer" (
    set "BUILD_TARGET=analyzer_aggressive_instruction_finder_tests analyzer_apply_data_archives_tests analyzer_ascii_strings_tests analyzer_call_convention_id_tests analyzer_call_fixup_installer_tests analyzer_condense_filler_bytes_tests analyzer_create_address_tables_tests analyzer_decompiler_parameter_id_tests analyzer_decompiler_switch_analysis_tests analyzer_demangler_microsoft_tests analyzer_disassemble_entry_points_tests analyzer_embedded_media_tests analyzer_external_entry_references_tests analyzer_function_id_tests analyzer_function_start_search_tests analyzer_non_returning_functions_tests analyzer_pdb_msdia_tests analyzer_pdb_universal_tests analyzer_reference_tests analyzer_scalar_operand_references_tests analyzer_shared_return_calls_tests analyzer_shared_function_body_tests analyzer_stack_tests analyzer_subroutine_references_tests analyzer_variadic_tests analyzer_winparams_tests analyzer_windows_resource_reference_tests analyzer_x86_constant_reference_tests analyzer_constant_propagation_tests analyzer_data_reference_tests analyzer_global_integration_tests"
    set "TEST_FILTER=^analyzer_(aggressive_instruction_finder|apply_data_archives|ascii_strings|call_convention_id|call_fixup_installer|condense_filler_bytes|create_address_tables|decompiler_parameter_id|decompiler_switch_analysis|demangler_microsoft|disassemble_entry_points|embedded_media|external_entry_references|function_id|function_start_search|non_returning_functions|pdb_msdia|pdb_universal|reference|scalar_operand_references|shared_return_calls|shared_function_body|stack|subroutine_references|variadic_function_signature_override|windows_pe_x86_propagate_external_parameters|windows_resource_reference|x86_constant_reference|constant_propagation|data_reference|global_integration)_tests$"
    goto mode_selected
)
if /I "%MODE%"=="analyzer_global_integration" (
    set "BUILD_TARGET=analyzer_global_integration_tests"
    set "TEST_FILTER=^analyzer_global_integration_tests$"
    goto mode_selected
)
goto usage

:mode_selected
if /I "%~2"=="--no-test" set "RUN_TESTS=0"
if /I "%~2"=="--clean" (
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)
if /I "%~3"=="--clean" (
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

if "%RUN_TESTS%"=="0" (
    if /I "%MODE%"=="hello" set "BUILD_TARGET=hello_feature"
    if /I "%MODE%"=="sleigh" set "BUILD_TARGET=sleigh_runtime"
    if /I "%MODE%"=="pe" set "BUILD_TARGET=pe_loader"
    if /I "%MODE%"=="function_id" set "BUILD_TARGET=function_id function_id_cli"
    if /I "%MODE%"=="decompiler" set "BUILD_TARGET=new_ghidra_decompiler_frontend decompiler_cli"
    if /I "%MODE%"=="analyzer" set "BUILD_TARGET=analyzer"
    if /I "%MODE%"=="constant_propagation" set "BUILD_TARGET=analyzer_constant_propagation"
    if /I "%MODE%"=="data_reference" set "BUILD_TARGET=analyzer_data_reference"
    if /I "%MODE%"=="disassemble_entry_points" set "BUILD_TARGET=analyzer_disassemble_entry_points"
    if /I "%MODE%"=="shared_function_body" set "BUILD_TARGET=analyzer_shared"
    if /I "%MODE%"=="function_start_search" set "BUILD_TARGET=analyzer_function_start_search"
    if /I "%MODE%"=="non_returning_functions" set "BUILD_TARGET=analyzer_non_returning_functions"
    if /I "%MODE%"=="reference" set "BUILD_TARGET=analyzer_reference"
    if /I "%MODE%"=="scalar_operand_references" set "BUILD_TARGET=analyzer_scalar_operand_references"
    if /I "%MODE%"=="stack" set "BUILD_TARGET=analyzer_stack"
    if /I "%MODE%"=="subroutine_references" set "BUILD_TARGET=analyzer_subroutine_references"
    if /I "%MODE%"=="aggressive_instruction_finder" set "BUILD_TARGET=analyzer_aggressive_instruction_finder"
    if /I "%MODE%"=="apply_data_archives" set "BUILD_TARGET=analyzer_apply_data_archives"
    if /I "%MODE%"=="ascii_strings" set "BUILD_TARGET=analyzer_ascii_strings"
    if /I "%MODE%"=="call_convention_id" set "BUILD_TARGET=analyzer_call_convention_id"
    if /I "%MODE%"=="call_fixup_installer" set "BUILD_TARGET=analyzer_call_fixup_installer"
    if /I "%MODE%"=="condense_filler_bytes" set "BUILD_TARGET=analyzer_condense_filler_bytes"
    if /I "%MODE%"=="create_address_tables" set "BUILD_TARGET=analyzer_create_address_tables"
    if /I "%MODE%"=="decompiler_parameter_id" set "BUILD_TARGET=analyzer_decompiler_parameter_id"
    if /I "%MODE%"=="decompiler_switch_analysis" set "BUILD_TARGET=analyzer_decompiler_switch_analysis"
    if /I "%MODE%"=="demangler_microsoft" set "BUILD_TARGET=analyzer_demangler_microsoft"
    if /I "%MODE%"=="embedded_media" set "BUILD_TARGET=analyzer_embedded_media"
    if /I "%MODE%"=="external_entry_references" set "BUILD_TARGET=analyzer_external_entry_references"
    if /I "%MODE%"=="function_id_analyzer" set "BUILD_TARGET=analyzer_function_id"
    if /I "%MODE%"=="pdb_msdia" set "BUILD_TARGET=analyzer_pdb_msdia"
    if /I "%MODE%"=="pdb_universal" set "BUILD_TARGET=analyzer_pdb_universal"
    if /I "%MODE%"=="shared_return_calls" set "BUILD_TARGET=analyzer_shared_return_calls"
     if /I "%MODE%"=="variadic_function_signature_override" set "BUILD_TARGET=analyzer_variadic"
     if /I "%MODE%"=="windows_pe_x86_propagate_external_parameters" set "BUILD_TARGET=analyzer_winparams"
    if /I "%MODE%"=="windows_resource_reference" set "BUILD_TARGET=analyzer_windows_resource_reference"
    if /I "%MODE%"=="x86_constant_reference" set "BUILD_TARGET=analyzer_x86_constant_reference"
    if /I "%MODE%"=="analyzer_global_integration" set "BUILD_TARGET=analyzer_global_integration_tests"
    if /I "%MODE%"=="shared" set "BUILD_TARGET=analyzer_shared"
)

if not defined VCPKG_ROOT (
    echo ERROR: VCPKG_ROOT is not set.
    echo Set VCPKG_ROOT to the vcpkg installation directory and retry.
    exit /b 1
)

set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
    echo ERROR: Visual Studio C++ tools were not found.
    exit /b 1
)
call "%VSDEVCMD%" -arch=x64
if not defined VSCMD_ARG_TGT_ARCH (
    echo ERROR: Visual Studio C++ tools were not found.
    exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo ERROR: vcpkg was not found at "%VCPKG_ROOT%".
    echo Set VCPKG_ROOT to the vcpkg installation directory and retry.
    exit /b 1
)

set "CMAKE_EXE=%VCPKG_ROOT%\downloads\tools\cmake-4.4.2-windows\cmake-4.4.2-windows-x86_64\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=cmake"

"%CMAKE_EXE%" -S "%SCRIPT_DIR%." -B "%SCRIPT_DIR%build" -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DBUILD_TESTING=%RUN_TESTS%
if errorlevel 1 exit /b 1

if "%FULL_BUILD%"=="1" (
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel
) else (
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target %BUILD_TARGET% --parallel
)
if errorlevel 1 exit /b 1

if "%RUN_TESTS%"=="1" (
    if "%FULL_BUILD%"=="1" (
        "%CMAKE_EXE%" -E env CTEST_OUTPUT_ON_FAILURE=1 ctest --test-dir "%BUILD_DIR%" --parallel
    ) else (
        "%CMAKE_EXE%" -E env CTEST_OUTPUT_ON_FAILURE=1 ctest --test-dir "%BUILD_DIR%" -R "%TEST_FILTER%" --parallel
    )
    if errorlevel 1 exit /b 1
)

echo Build completed successfully.
echo Build mode: %MODE%
echo Build directory: "%BUILD_DIR%"
endlocal
exit /b 0

:usage
echo Usage: build.bat [app^|hello^|sleigh^|pe^|function_id^|decompiler^|analyzer^|analyzer_global_integration^|shared_function_body^|all] [--no-test] [--clean]
echo.
echo Default mode: all. The build directory is preserved for fast incremental builds.
echo Use a module mode to build and test only that module.
echo Use all to build every target and run every registered test.
echo Use --clean only when a clean rebuild is required.
endlocal
exit /b 0
