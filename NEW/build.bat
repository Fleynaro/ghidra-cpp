@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "VCPKG_ROOT=%VCPKG_ROOT%"
if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\dev\vcpkg"

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

if exist "%SCRIPT_DIR%build" rmdir /s /q "%SCRIPT_DIR%build"

set "CMAKE_EXE=%VCPKG_ROOT%\downloads\tools\cmake-4.4.2-windows\cmake-4.4.2-windows-x86_64\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=cmake"

"%CMAKE_EXE%" -S "%SCRIPT_DIR%." -B "%SCRIPT_DIR%build" -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows
if errorlevel 1 exit /b 1

"%CMAKE_EXE%" --build "%SCRIPT_DIR%build" --parallel
if errorlevel 1 exit /b 1

"%CMAKE_EXE%" -E env CTEST_OUTPUT_ON_FAILURE=1 ctest --test-dir "%SCRIPT_DIR%build"
if errorlevel 1 exit /b 1

echo Build completed successfully.
echo Executable: "%SCRIPT_DIR%build\new_ghidra_app.exe"
endlocal
