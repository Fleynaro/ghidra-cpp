@echo off
setlocal EnableExtensions

rem Restore the pinned Microsoft TTD dependency workflow owned by this service.
set "SERVICE_DIR=%~dp0"
set "DEPENDENCY_ROOT=%SERVICE_DIR%dependencies"
set "PACKAGES_CONFIG=%DEPENDENCY_ROOT%\packages.config"
set "PACKAGES_DIR=%DEPENDENCY_ROOT%"
set "RUNTIME_DIR=%DEPENDENCY_ROOT%\runtime\x64"
set "NUGET_EXE=%NUGET_EXE%"

if not exist "%PACKAGES_CONFIG%" (
    echo ERROR: Reference NuGet manifest was not found: "%PACKAGES_CONFIG%"
    exit /b 1
)

if defined NUGET_EXE goto nuget_ready
for /f "delims=" %%I in ('where nuget.exe 2^>nul') do if not defined NUGET_EXE set "NUGET_EXE=%%I"
if defined NUGET_EXE goto nuget_ready
set "NUGET_EXE=%TEMP%\ghidra-nuget.exe"
if exist "%NUGET_EXE%" goto nuget_ready
echo NuGet was not found on PATH. Downloading the official command-line client...
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile (Join-Path $env:TEMP 'ghidra-nuget.exe')"
if errorlevel 1 (
    echo ERROR: NuGet download failed.
    exit /b 1
)

:nuget_ready

echo Restoring Microsoft TTD API and debugger packages...
"%NUGET_EXE%" restore "%PACKAGES_CONFIG%" -PackagesDirectory "%PACKAGES_DIR%"
if errorlevel 1 (
    echo ERROR: NuGet restore failed.
    exit /b 1
)

echo Downloading and staging TTDReplay.dll and TTDReplayCPU.dll...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%DEPENDENCY_ROOT%\Get-TtdReplayRuntime.ps1" -OutDir "%RUNTIME_DIR%" -Arch x64
if errorlevel 1 (
    echo ERROR: TTD runtime staging failed.
    exit /b 1
)

if not exist "%DEPENDENCY_ROOT%\Microsoft.TimeTravelDebugging.Apis.0.9.5\CMake" (
    echo ERROR: Microsoft.TimeTravelDebugging.Apis.0.9.5 was not restored.
    exit /b 1
)
if not exist "%RUNTIME_DIR%\TTDReplay.dll" (
    echo ERROR: TTDReplay.dll was not staged.
    exit /b 1
)
if not exist "%RUNTIME_DIR%\TTDReplayCPU.dll" (
    echo ERROR: TTDReplayCPU.dll was not staged.
    exit /b 1
)

echo TTD dependencies are ready:
echo   TTD_APIS_PACKAGE_DIR=%DEPENDENCY_ROOT%\Microsoft.TimeTravelDebugging.Apis.0.9.5
echo   TTD_RUNTIME_DIR=%RUNTIME_DIR%
echo.
echo Run from the repository root with:
echo   set TTD_APIS_PACKAGE_DIR=%DEPENDENCY_ROOT%\Microsoft.TimeTravelDebugging.Apis.0.9.5
echo   set TTD_RUNTIME_DIR=%RUNTIME_DIR%
echo   NEW\build.bat ttd_replay
endlocal
exit /b 0
