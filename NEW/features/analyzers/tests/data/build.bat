@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the single native executable used by the complete analyzer integration test.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is unavailable and vswhere.exe was not found.
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

where rc >nul 2>&1
if errorlevel 1 (
    echo ERROR: rc.exe is unavailable. Install the Windows SDK resource compiler.
    exit /b 1
)

rc /nologo /I . /fo test_analyzers_integration.res test_analyzers_integration.rc
if errorlevel 1 exit /b 1

cl /nologo /c test_analyzers_integration.cpp /Fo:test_analyzers_integration.obj ^
    /Zi /O2 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_analyzers_integration.obj test_analyzers_integration.res kernel32.lib user32.lib ^
    /OUT:test_analyzers_integration.exe /PDB:test_analyzers_integration.pdb /DEBUG:FULL ^
    /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF ^
    /INCREMENTAL:NO /BREPRO
if errorlevel 1 exit /b 1
if not exist test_analyzers_integration.pdb (
    echo ERROR: The linker did not produce test_analyzers_integration.pdb.
    exit /b 1
)

del /q test_analyzers_integration.obj test_analyzers_integration.res >nul 2>&1
del /q test_analyzers_integration.lib test_analyzers_integration.exp vc140.pdb >nul 2>&1
echo Built test_analyzers_integration.exe and .pdb
exit /b 0
