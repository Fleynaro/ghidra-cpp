@echo off
call "%~dp0..\..\build.bat" pe %*
exit /b %errorlevel%
