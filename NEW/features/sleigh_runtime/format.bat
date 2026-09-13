@echo off
call "%~dp0..\..\format.bat" sleigh %*
exit /b %errorlevel%
