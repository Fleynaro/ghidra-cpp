@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Run a Python Ghidra script with the user venv created by PyGhidra.

if "%GHIDRA_INSTALL_DIR%"=="" (
    echo ERROR: GHIDRA_INSTALL_DIR is not set.
    exit /b 1
)

if not exist "%GHIDRA_INSTALL_DIR%\Ghidra\application.properties" (
    echo ERROR: Ghidra installation was not found at "%GHIDRA_INSTALL_DIR%".
    exit /b 1
)

set "GHIDRA_PYTHON="
for /d %%D in ("%APPDATA%\ghidra\ghidra_*") do (
    if exist "%%~fD\venv\Scripts\python.exe" if exist "%%~fD\venv\Lib\site-packages\pyghidra\__init__.py" (
        set "GHIDRA_PYTHON=%%~fD\venv\Scripts\python.exe"
    )
)

if not defined GHIDRA_PYTHON (
    echo ERROR: A PyGhidra virtual environment was not found under "%APPDATA%\ghidra".
    echo Remediation: run "%GHIDRA_INSTALL_DIR%\support\pyghidraRun.bat" once and install PyGhidra into its venv.
    exit /b 1
)

if "%~1"=="" (
    echo Usage: %~nx0 ^<script.py^> [script arguments...]
    exit /b 2
)

set "SCRIPT_PATH=%~1"
if not exist "%SCRIPT_PATH%" set "SCRIPT_PATH=%~dp0%~1"
if not exist "%SCRIPT_PATH%" (
    echo ERROR: Python script was not found: "%~1".
    exit /b 2
)

shift
rem %* is not updated by SHIFT, so forward the remaining supported arguments explicitly.
"%GHIDRA_PYTHON%" "%SCRIPT_PATH%" %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%
