@echo off
call "%~dp0..\..\..\build.bat" x86_constant_reference %*
exit /b %errorlevel%
