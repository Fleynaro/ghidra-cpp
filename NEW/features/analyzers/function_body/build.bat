@echo off
call "%~dp0..\..\..\build.bat" function_body %*
exit /b %errorlevel%
