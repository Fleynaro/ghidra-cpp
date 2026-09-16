@echo off
call "%~dp0..\..\build.bat" shared --no-test %*
exit /b %errorlevel%
