@echo off
call "%~dp0..\..\format.bat" decompiler %*
exit /b %errorlevel%
