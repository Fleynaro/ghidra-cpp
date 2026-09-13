@echo off
call "%~dp0..\..\build.bat" decompiler %*
exit /b %errorlevel%
