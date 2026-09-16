@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the deterministic CRT-free MSVC x64 Reference fixture.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe and vswhere.exe are unavailable.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL exit /b 1
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)
cl /nologo /c test_reference.cpp /Fo:test_reference.obj /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_reference.obj /OUT:test_reference.exe /SUBSYSTEM:CONSOLE /ENTRY:reference_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /Brepro
if errorlevel 1 exit /b 1
del /q test_reference.obj test_reference.lib test_reference.exp >nul 2>&1
echo Built test_reference.exe
exit /b 0
