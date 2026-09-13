@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build an optimized CRT-free MSVC x64 Shared Return Calls fixture.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" exit /b 1
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL exit /b 1
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)
cl /nologo /c test_shared_return_calls.cpp /Fo:test_shared_return_calls.obj /O2 /Oi- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_shared_return_calls.obj /OUT:test_shared_return_calls.exe /SUBSYSTEM:CONSOLE /ENTRY:shared_return_calls_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /Brepro
if errorlevel 1 exit /b 1
del /q test_shared_return_calls.obj test_shared_return_calls.lib test_shared_return_calls.exp >nul 2>&1
echo Built test_shared_return_calls.exe
exit /b 0
