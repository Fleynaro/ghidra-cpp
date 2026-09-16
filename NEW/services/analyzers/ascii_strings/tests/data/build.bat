@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a deterministic, CRT-free MSVC x64 PE for StringsAnalyzer.

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

cl /nologo /c test_ascii_strings.cpp /Fo:test_ascii_strings.obj ^
    /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- ^
    /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_ascii_strings.obj /OUT:test_ascii_strings.exe ^
    /SUBSYSTEM:CONSOLE /ENTRY:ascii_strings_entry /NODEFAULTLIB ^
    /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /BREPRO ^
    /EXPORT:ascii_strings_entry
if errorlevel 1 exit /b 1

del /q test_ascii_strings.obj >nul 2>&1
del /q test_ascii_strings.lib test_ascii_strings.exp >nul 2>&1
echo Built test_ascii_strings.exe
exit /b 0
