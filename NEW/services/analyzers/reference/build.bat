@echo off
call "%~dp0..\..\..\build.bat" reference %*
exit /b %errorlevel%
