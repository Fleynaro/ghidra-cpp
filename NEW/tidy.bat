@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
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

set "NEED_BUILD=0"
if not exist "%BUILD_DIR%\compile_commands.json" set "NEED_BUILD=1"
if not exist "%BUILD_DIR%\features\hello\CMakeFiles\hello_feature.dir\hello.ifc" (
    set "NEED_BUILD=1"
)
if not exist "%BUILD_DIR%\features\sleigh_runtime\CMakeFiles\sleigh_runtime.dir\sleigh_runtime.ifc" (
    set "NEED_BUILD=1"
)

if "%NEED_BUILD%"=="1" (
    echo Required build artifacts were not found. Building NEW before clang-tidy...
    call "%SCRIPT_DIR%build.bat"
    if errorlevel 1 exit /b 1
)

if not exist "%BUILD_DIR%\compile_commands.json" (
    echo ERROR: "%BUILD_DIR%\compile_commands.json" was not generated.
    exit /b 1
)

echo Applying clang-tidy fixes to C and C++ translation units under "%SCRIPT_DIR%"...
echo Headers are analyzed when included by these translation units.
for /r "%SCRIPT_DIR%" %%F in (*.c *.cc *.cpp *.cxx *.cppm) do (
    findstr /r /c:"^[ ]*import [A-Za-z_][A-Za-z0-9_]*;" "%%~fF" >nul 2>&1
    if not errorlevel 1 (
        echo SKIP: module consumer cannot be parsed by clang-tidy with MSVC .ifc files: "%%~fF".
        set /a SKIPPED_COUNT+=1
    ) else (
        set /a FILE_COUNT+=1
        clang-tidy "%%~fF" -p "%BUILD_DIR%" -fix -quiet -extra-arg=-Wno-unused-command-line-argument
        if errorlevel 1 (
            echo ERROR: clang-tidy failed for "%%~fF".
            set /a ERROR_COUNT+=1
        )
    )
)

if %ERROR_COUNT% gtr 0 (
    echo clang-tidy failed for %ERROR_COUNT% of %FILE_COUNT% files.
    exit /b 1
)

echo clang-tidy processed %FILE_COUNT% files successfully and skipped %SKIPPED_COUNT% module consumers.
endlocal
