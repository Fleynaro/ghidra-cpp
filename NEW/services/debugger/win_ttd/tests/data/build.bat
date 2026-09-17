@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a real, symbolized x64 process for deterministic debugger integration tests.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is not available and vswhere.exe was not found.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL (
        echo ERROR: An MSVC installation with x64 tools was not found.
        exit /b 1
    )
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)

cl /nologo /std:c++20 /EHsc /Zi /Od /Ob0 /W4 /WX /MDd /D_DEBUG /DWIN32_LEAN_AND_MEAN ^
    debugger_debuggee.cpp /Fe:debugger_debuggee.exe /link /DEBUG:FULL /INCREMENTAL:NO
if errorlevel 1 exit /b 1

del /q debugger_debuggee.obj >nul 2>&1
echo Built debugger_debuggee.exe
exit /b 0
