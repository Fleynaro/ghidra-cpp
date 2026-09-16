@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "MODE=%~1"
if not defined MODE set "MODE=all"
set "SOURCE_DIR="
set "CHECK_ONLY=0"
if /I "%MODE%"=="--check" (
    set "CHECK_ONLY=1"
    set "MODE=all"
)
if /I "%~2"=="--check" set "CHECK_ONLY=1"

if /I "%MODE%"=="all" set "SOURCE_DIR=%SCRIPT_DIR%"
if /I "%MODE%"=="hello" set "SOURCE_DIR=%SCRIPT_DIR%features\hello"
if /I "%MODE%"=="sleigh" set "SOURCE_DIR=%SCRIPT_DIR%features\sleigh_runtime"
if /I "%MODE%"=="pe" set "SOURCE_DIR=%SCRIPT_DIR%features\pe_loader"
if /I "%MODE%"=="function_id" set "SOURCE_DIR=%SCRIPT_DIR%features\function_id"
if /I "%MODE%"=="decompiler" set "SOURCE_DIR=%SCRIPT_DIR%features\decompiler"
if /I "%MODE%"=="analyzer" set "SOURCE_DIR=%SCRIPT_DIR%features\analyzers"
if /I "%MODE%"=="core" set "SOURCE_DIR=%SCRIPT_DIR%core"
if /I "%MODE%"=="runtime" set "SOURCE_DIR=%SCRIPT_DIR%runtime"
if /I "%MODE%"=="services" set "SOURCE_DIR=%SCRIPT_DIR%services"
if /I "%MODE%"=="tests" set "SOURCE_DIR=%SCRIPT_DIR%tests"
if /I "%MODE%"=="bindings" set "SOURCE_DIR=%SCRIPT_DIR%bindings"
if not defined SOURCE_DIR goto usage

set "BUILD_DIR=%SCRIPT_DIR%build"
set /a FILE_COUNT=0
set /a ERROR_COUNT=0
set /a SKIPPED_COUNT=0

where clang-tidy >nul 2>&1
if errorlevel 1 (
    echo ERROR: clang-tidy was not found on PATH.
    echo Install LLVM or add its bin directory to PATH, then retry.
    exit /b 1
)

if not exist "%BUILD_DIR%\compile_commands.json" (
    echo Required build artifacts were not found. Building NEW before clang-tidy...
    call "%SCRIPT_DIR%build.bat" %MODE%
    if errorlevel 1 exit /b 1
)
if not exist "%BUILD_DIR%\compile_commands.json" (
    echo ERROR: "%BUILD_DIR%\compile_commands.json" was not generated.
    exit /b 1
)

if "%CHECK_ONLY%"=="1" (
    echo Checking C and C++ translation units under "%SOURCE_DIR%" without applying fixes...
) else (
    echo Applying clang-tidy fixes to C and C++ translation units under "%SOURCE_DIR%"...
)
echo Headers are analyzed when included by these translation units.
for /r "%SOURCE_DIR%" %%F in (*.c *.cc *.cpp *.cxx *.cppm) do (
    findstr /r /c:"^[ ]*import " /c:"^[ ]*export import " "%%~fF" >nul 2>&1
    if not errorlevel 1 (
        echo SKIP: module consumer cannot be parsed by clang-tidy with MSVC .ifc files: "%%~fF".
        set /a SKIPPED_COUNT+=1
    ) else (
        set /a FILE_COUNT+=1
        if "%CHECK_ONLY%"=="1" (
            clang-tidy "%%~fF" -p "%BUILD_DIR%" -quiet -extra-arg=-Wno-unused-command-line-argument
        ) else (
            clang-tidy "%%~fF" -p "%BUILD_DIR%" -fix -quiet -extra-arg=-Wno-unused-command-line-argument
        )
        if errorlevel 1 (
            echo ERROR: clang-tidy failed for "%%~fF".
            set /a ERROR_COUNT+=1
        )
    )
)

if !ERROR_COUNT! gtr 0 (
    echo clang-tidy failed for !ERROR_COUNT! of !FILE_COUNT! files.
    exit /b 1
)
echo clang-tidy processed !FILE_COUNT! files successfully and skipped !SKIPPED_COUNT! module consumers.
endlocal
exit /b 0

:usage
echo Usage: tidy.bat [all^|hello^|sleigh^|pe^|function_id^|decompiler^|analyzer^|core^|runtime^|services^|tests^|bindings] [--check]
echo.
echo Default mode: all. Without --check, clang-tidy applies fixes.
echo Use --check to analyze files without modifying source files.
endlocal
exit /b 2
