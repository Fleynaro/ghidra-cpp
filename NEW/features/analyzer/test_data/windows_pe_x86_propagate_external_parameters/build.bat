@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the mandatory 32-bit MSVC PE used by the x86 PUSH-based analyzer.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is unavailable and vswhere.exe was not found.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL (
        echo ERROR: An MSVC installation with x86 tools was not found.
        exit /b 1
    )
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x86
    if errorlevel 1 exit /b 1
)

cl /nologo /c test_windows_pe_x86_propagate_external_parameters.cpp /Fo:test_windows_pe_x86_propagate_external_parameters.obj ^
    /Zi /Od /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_windows_pe_x86_propagate_external_parameters.obj user32.lib ^
    /OUT:test_windows_pe_x86_propagate_external_parameters.exe ^
    /PDB:test_windows_pe_x86_propagate_external_parameters.pdb /DEBUG:FULL ^
    /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB /FIXED:NO ^
    /BASE:0x00400000 /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /Brepro
if errorlevel 1 exit /b 1

del /q test_windows_pe_x86_propagate_external_parameters.obj >nul 2>&1
del /q test_windows_pe_x86_propagate_external_parameters.lib test_windows_pe_x86_propagate_external_parameters.exp vc140.pdb >nul 2>&1
echo Built test_windows_pe_x86_propagate_external_parameters.exe and .pdb
exit /b 0
