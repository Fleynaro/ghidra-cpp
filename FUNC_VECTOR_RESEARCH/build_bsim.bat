@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build the combined database and input PE fixtures for the BSim research.
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
    if errorlevel 1 (
        popd
        exit /b 1
    )
)

cl /nologo /std:c++latest /EHsc /Zi /Od /Ob0 /W4 /WX /MDd /D_DEBUG database.cpp /Fe:database.exe /link /DEBUG:FULL /INCREMENTAL:NO
if errorlevel 1 goto :failed
cl /nologo /std:c++latest /EHsc /Zi /Od /Ob0 /W4 /WX /MDd /D_DEBUG input.cpp /Fe:input.exe /link /DEBUG:FULL /INCREMENTAL:NO
if errorlevel 1 goto :failed
if not exist database.exe goto :failed
if not exist input.exe goto :failed

del /q database.obj input.obj database.lib database.exp input.lib input.exp vc140.pdb >nul 2>&1
echo Built database.exe and input.exe
popd
exit /b 0

:failed
echo ERROR: Could not build both BSim research executables.
popd
exit /b 1
