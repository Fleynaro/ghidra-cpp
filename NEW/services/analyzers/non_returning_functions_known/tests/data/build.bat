@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a CRT-free MSVC x64 PE with a real exported abort symbol.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is unavailable and vswhere.exe was not found.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL exit /b 1
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)

cl /nologo /c test_non_returning_functions_known.cpp /Fo:test_non_returning_functions_known.obj /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_non_returning_functions_known.obj /OUT:test_non_returning_functions_known.exe /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /Brepro
if errorlevel 1 exit /b 1
del /q test_non_returning_functions_known.obj test_non_returning_functions_known.lib test_non_returning_functions_known.exp >nul 2>&1
echo Built test_non_returning_functions_known.exe
exit /b 0
