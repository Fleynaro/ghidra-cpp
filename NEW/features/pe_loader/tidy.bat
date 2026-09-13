@echo off
call "%~dp0..\..\tidy.bat" pe %*
exit /b %errorlevel%
