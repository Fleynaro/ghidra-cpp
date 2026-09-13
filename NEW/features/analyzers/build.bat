@echo off
call "%~dp0..\..\build.bat" analyzer %*
exit /b %errorlevel%
