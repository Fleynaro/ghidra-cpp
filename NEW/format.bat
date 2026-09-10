@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set /a FILE_COUNT=0
set /a ERROR_COUNT=0

where clang-format >nul 2>&1
if errorlevel 1 (
    echo ERROR: clang-format was not found on PATH.
    echo Install LLVM or add its bin directory to PATH, then retry.
    exit /b 1
)

echo Formatting C and C++ files under "%SCRIPT_DIR%"...
for /r "%SCRIPT_DIR%" %%F in (*.c *.cc *.cpp *.cxx *.h *.hh *.hpp *.hxx *.cppm *.inl *.ipp) do (
    set /a FILE_COUNT+=1
    clang-format --style=file --fallback-style=Microsoft -i "%%~fF"
    if errorlevel 1 (
        echo ERROR: failed to format "%%~fF".
        set /a ERROR_COUNT+=1
    )
)

if %ERROR_COUNT% gtr 0 (
    echo Formatting failed for %ERROR_COUNT% of %FILE_COUNT% files.
    exit /b 1
)

echo Formatted %FILE_COUNT% files successfully.
endlocal
