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
if /I "%MODE%"=="analyzer" (
    set "BUILD_TARGET=analyzer_tests"
    set "TEST_FILTER=^analyzer_tests$"
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
echo Usage: build.bat [app^|hello^|sleigh^|pe^|function_id^|decompiler^|analyzer^|all] [--no-test] [--clean]
echo.
echo Default mode: all. The build directory is preserved for fast incremental builds.
echo Use a module mode to build and test only that module.
echo Use all to build every target and run every registered test.
echo Use --clean only when a clean rebuild is required.
endlocal
exit /b 0
