@echo off
call "%~dp0..\..\..\build.bat" windows_resource_reference %*
exit /b %errorlevel%
