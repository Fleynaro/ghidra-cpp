@echo off
call "%~dp0..\..\build.bat" sleigh %*
exit /b %errorlevel%
