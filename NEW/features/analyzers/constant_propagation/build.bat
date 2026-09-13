@echo off
call "%~dp0..\..\..\build.bat" constant_propagation %*
exit /b %errorlevel%
