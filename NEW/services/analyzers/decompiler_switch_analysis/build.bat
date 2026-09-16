@echo off
call "%~dp0..\..\..\build.bat" decompiler_switch_analysis %*
exit /b %errorlevel%
