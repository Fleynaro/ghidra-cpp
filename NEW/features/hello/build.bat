@echo off
call "%~dp0..\..\build.bat" hello %*
exit /b %errorlevel%
