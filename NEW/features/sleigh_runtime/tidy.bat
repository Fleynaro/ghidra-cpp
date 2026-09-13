@echo off
call "%~dp0..\..\tidy.bat" sleigh %*
exit /b %errorlevel%
