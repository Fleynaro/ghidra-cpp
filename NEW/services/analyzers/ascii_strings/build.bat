@echo off
call "%~dp0..\..\..\build.bat" ascii_strings %*
exit /b %errorlevel%
