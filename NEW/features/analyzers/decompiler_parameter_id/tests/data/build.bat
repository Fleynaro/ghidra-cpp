@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a deterministic CRT-free MSVC x64 PE for Decompiler Parameter ID.

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

cl /nologo /c test_decompiler_parameter_id.cpp /Fo:test_decompiler_parameter_id.obj ^
    /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /Zi /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_decompiler_parameter_id.obj /OUT:test_decompiler_parameter_id.exe ^
    /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF ^
    /PDB:test_decompiler_parameter_id.pdb /DEBUG:FULL /INCREMENTAL:NO /Brepro
if errorlevel 1 exit /b 1
del /q test_decompiler_parameter_id.obj test_decompiler_parameter_id.lib test_decompiler_parameter_id.exp vc140.pdb >nul 2>&1
echo Built test_decompiler_parameter_id.exe and test_decompiler_parameter_id.pdb
exit /b 0
