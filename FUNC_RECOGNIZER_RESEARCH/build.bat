@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the ten-function x64 PE corpus with symbols and no optimization.
pushd "%~dp0"
if errorlevel 1 exit /b 1

where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is unavailable and vswhere.exe was not found.
        popd
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL (
        echo ERROR: An MSVC installation with x64 tools was not found.
        popd
        exit /b 1
    )
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 goto :failed
)

cl /nologo /std:c++latest /EHsc /Zi /Od /Ob0 /W4 /WX /MD /GS- src\corpus.cpp /Fe:corpus.exe /link /DEBUG:FULL /INCREMENTAL:NO /OPT:NOREF
if errorlevel 1 goto :failed
if not exist corpus.exe goto :failed
del /q corpus.obj corpus.pdb corpus.lib corpus.exp vc140.pdb >nul 2>&1
echo Built corpus.exe
popd
exit /b 0

:failed
echo ERROR: Could not build the P-Code research corpus.
popd
exit /b 1
