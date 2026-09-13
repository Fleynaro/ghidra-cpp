@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a deterministic, CRT-free MSVC x64 PE for DecompilerCallConventionAnalyzer.

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

cl /nologo /c test_call_convention_id.cpp /Fo:test_call_convention_id.obj ^
    /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- ^
    /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_call_convention_id.obj /OUT:test_call_convention_id.exe ^
    /SUBSYSTEM:CONSOLE /ENTRY:call_convention_id_entry /NODEFAULTLIB ^
    /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /BREPRO ^
    /EXPORT:call_convention_id_entry /EXPORT:convention_target
if errorlevel 1 exit /b 1

del /q test_call_convention_id.obj >nul 2>&1
del /q test_call_convention_id.lib test_call_convention_id.exp >nul 2>&1
echo Built test_call_convention_id.exe
exit /b 0
