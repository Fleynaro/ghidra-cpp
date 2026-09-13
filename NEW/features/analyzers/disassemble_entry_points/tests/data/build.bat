@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a deterministic CRT-free MSVC x64 PE for Disassemble Entry Points.

where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is not available and vswhere.exe was not found.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL exit /b 1
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)

cl /nologo /c test_disassemble_entry_points.cpp /Fo:test_disassemble_entry_points.obj ^
    /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_disassemble_entry_points.obj /OUT:test_disassemble_entry_points.exe ^
    /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF ^
    /INCREMENTAL:NO /DEBUG:NONE /Brepro
if errorlevel 1 exit /b 1
del /q test_disassemble_entry_points.obj test_disassemble_entry_points.lib test_disassemble_entry_points.exp >nul 2>&1
echo Built test_disassemble_entry_points.exe
exit /b 0
