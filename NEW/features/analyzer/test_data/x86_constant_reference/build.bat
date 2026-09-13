@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the mandatory 32-bit MSVC PE used by X86Analyzer.java.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    echo ERROR: vswhere.exe was not found; an x86 MSVC environment is required.
    exit /b 1
)
set "VSINSTALL="
for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL (
    echo ERROR: An MSVC installation with x86 tools was not found.
    exit /b 1
)
call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x86
if errorlevel 1 exit /b 1

cl /nologo /c test_x86_constant_reference.cpp /Fo:test_x86_constant_reference.obj ^
    /Zi /Od /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_x86_constant_reference.obj /OUT:test_x86_constant_reference.exe ^
    /PDB:test_x86_constant_reference.pdb /DEBUG:FULL /SUBSYSTEM:CONSOLE ^
    /ENTRY:fixture_entry /NODEFAULTLIB /FIXED:NO /BASE:0x00400000 ^
    /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /Brepro
if errorlevel 1 exit /b 1

if not exist test_x86_constant_reference.pdb (
    echo ERROR: The x86 linker did not produce test_x86_constant_reference.pdb.
    exit /b 1
)
del /q test_x86_constant_reference.obj >nul 2>&1
del /q test_x86_constant_reference.lib test_x86_constant_reference.exp vc140.pdb >nul 2>&1
echo Built test_x86_constant_reference.exe and .pdb
exit /b 0
