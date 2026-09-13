@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a deterministic CRT-free MSVC x64 PE and retain its PDB type information.
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

cl /nologo /c test_variadic_function_signature_override.cpp /Fo:test_variadic_function_signature_override.obj ^
    /Zi /Od /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_variadic_function_signature_override.obj ^
    /OUT:test_variadic_function_signature_override.exe /PDB:test_variadic_function_signature_override.pdb ^
    /DEBUG:FULL /SUBSYSTEM:CONSOLE /ENTRY:fixture_entry /NODEFAULTLIB ^
    /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /Brepro
if errorlevel 1 exit /b 1

del /q test_variadic_function_signature_override.obj >nul 2>&1
del /q printf_fixture.lib printf_fixture.exp test_variadic_function_signature_override.lib test_variadic_function_signature_override.exp vc140.pdb >nul 2>&1
echo Built test_variadic_function_signature_override.exe and .pdb
exit /b 0
