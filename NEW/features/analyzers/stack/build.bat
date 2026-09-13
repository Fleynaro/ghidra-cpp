@echo off
call "%~dp0..\..\..\build.bat" stack %*
exit /b %errorlevel%
