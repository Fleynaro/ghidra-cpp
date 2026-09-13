@echo off
call "%~dp0..\..\tidy.bat" decompiler %*
exit /b %errorlevel%
