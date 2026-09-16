@echo off
call "%~dp0..\..\..\build.bat" demangler_microsoft %*
exit /b %errorlevel%
