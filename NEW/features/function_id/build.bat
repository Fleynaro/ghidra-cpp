@echo off
call "%~dp0..\..\build.bat" function_id %*
exit /b %errorlevel%
