@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the deterministic CRT-free MSVC x64 Scalar Operand References fixture.
where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" exit /b 1
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL exit /b 1
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)
cl /nologo /c test_scalar_operand_references.cpp /Fo:test_scalar_operand_references.obj /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- /W4 /WX /Brepro
if errorlevel 1 exit /b 1
link /nologo test_scalar_operand_references.obj /OUT:test_scalar_operand_references.exe /SUBSYSTEM:CONSOLE /ENTRY:scalar_operand_references_entry /NODEFAULTLIB /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /Brepro
if errorlevel 1 exit /b 1
del /q test_scalar_operand_references.obj test_scalar_operand_references.lib test_scalar_operand_references.exp >nul 2>&1
echo Built test_scalar_operand_references.exe
exit /b 0
