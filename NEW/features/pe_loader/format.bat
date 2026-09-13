@echo off
call "%~dp0..\..\format.bat" pe %*
exit /b %errorlevel%
