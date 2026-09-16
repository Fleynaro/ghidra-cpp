@echo off
call "%~dp0..\..\..\build.bat" data_reference %*
exit /b %errorlevel%
