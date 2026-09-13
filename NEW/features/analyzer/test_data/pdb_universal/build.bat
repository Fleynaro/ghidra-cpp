@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a matching MSVC x64 /Zi PE and raw PDB for the PDB Universal fixture.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe and vswhere.exe are unavailable.
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
cl /nologo /c test_pdb_universal.cpp /Fo:test_pdb_universal.obj /Fd:test_pdb_universal.pdb /Zi /Od /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_pdb_universal.obj /OUT:test_pdb_universal.exe /SUBSYSTEM:CONSOLE /ENTRY:pdb_universal_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:FULL /PDB:test_pdb_universal.pdb /Brepro
if errorlevel 1 exit /b 1
del /q test_pdb_universal.obj test_pdb_universal.lib test_pdb_universal.exp >nul 2>&1
echo Built test_pdb_universal.exe and matching test_pdb_universal.pdb
exit /b 0
