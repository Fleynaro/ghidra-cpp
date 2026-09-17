@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the standalone MSVC PE used by BSim fixture and signature tests.
pushd "%~dp0"
if errorlevel 1 exit /b 1

where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is unavailable and vswhere.exe was not found.
        popd
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL (
        echo ERROR: An MSVC installation with x64 tools was not found.
        popd
        exit /b 1
    )
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 (
        popd
        exit /b 1
    )
)

cl /nologo /std:c++latest /EHsc /Zi /Od /Ob0 /W4 /WX /MDd /D_DEBUG ^
    bsim_fixture.cpp /Fe:bsim_fixture.exe /link /DEBUG:FULL /INCREMENTAL:NO
if errorlevel 1 (
    popd
    exit /b 1
)
if not exist bsim_fixture.exe (
    echo ERROR: The linker did not produce bsim_fixture.exe.
    popd
    exit /b 1
)

del /q bsim_fixture.obj vc140.pdb >nul 2>&1
echo Built bsim_fixture.exe and bsim_fixture.pdb
popd
exit /b 0
