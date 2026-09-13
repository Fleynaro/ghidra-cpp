@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "MODE=%~1"
if not defined MODE set "MODE=all"
set "SOURCE_DIR="
if /I "%MODE%"=="all" set "SOURCE_DIR=%SCRIPT_DIR%"
if /I "%MODE%"=="hello" set "SOURCE_DIR=%SCRIPT_DIR%features\hello"
if /I "%MODE%"=="sleigh" set "SOURCE_DIR=%SCRIPT_DIR%features\sleigh_runtime"
if /I "%MODE%"=="pe" set "SOURCE_DIR=%SCRIPT_DIR%features\pe_loader"
if /I "%MODE%"=="function_id" set "SOURCE_DIR=%SCRIPT_DIR%features\function_id"
if /I "%MODE%"=="decompiler" set "SOURCE_DIR=%SCRIPT_DIR%features\decompiler"
if not defined SOURCE_DIR goto usage

set /a FILE_COUNT=0
set /a ERROR_COUNT=0
where clang-format >nul 2>&1
if errorlevel 1 (
    echo ERROR: clang-format was not found on PATH.
    echo Install LLVM or add its bin directory to PATH, then retry.
    exit /b 1
)

echo Formatting C and C++ files under "%SOURCE_DIR%"...
for /r "%SOURCE_DIR%" %%F in (*.c *.cc *.cpp *.cxx *.h *.hh *.hpp *.hxx *.cppm *.inl *.ipp) do (
    set /a FILE_COUNT+=1
    clang-format --style=file --fallback-style=Microsoft -i "%%~fF"
    if errorlevel 1 (
        echo ERROR: failed to format "%%~fF".
        set /a ERROR_COUNT+=1
    )
)
if !ERROR_COUNT! gtr 0 (
    echo Formatting failed for !ERROR_COUNT! of !FILE_COUNT! files.
    exit /b 1
)
echo Formatted !FILE_COUNT! files successfully.
endlocal
exit /b 0

:usage
echo Usage: format.bat [all^|hello^|sleigh^|pe^|function_id^|decompiler]
echo.
echo Default mode: all.
endlocal
exit /b 2
