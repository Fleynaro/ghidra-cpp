@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build x64 code and link resources emitted by the real Windows resource compiler.
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

rc /nologo /fo test_windows_resource_reference.res test_windows_resource_reference.rc
if errorlevel 1 exit /b 1

cl /nologo /c test_windows_resource_reference.cpp /Fo:test_windows_resource_reference.obj ^
    /Zi /Od /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_windows_resource_reference.obj test_windows_resource_reference.res user32.lib kernel32.lib ^
    /OUT:test_windows_resource_reference.exe /PDB:test_windows_resource_reference.pdb /DEBUG:FULL ^
    /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF ^
    /INCREMENTAL:NO /Brepro
if errorlevel 1 exit /b 1
if not exist test_windows_resource_reference.pdb (
    echo ERROR: The linker did not produce test_windows_resource_reference.pdb.
    exit /b 1
)

del /q test_windows_resource_reference.obj test_windows_resource_reference.res >nul 2>&1
del /q test_windows_resource_reference.lib test_windows_resource_reference.exp vc140.pdb >nul 2>&1
echo Built test_windows_resource_reference.exe and .pdb with rc.exe resources
exit /b 0
