@echo off
call "%~dp0..\..\..\build.bat" function_start_search %*
exit /b %errorlevel%
