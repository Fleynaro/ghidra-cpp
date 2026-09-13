@echo off
call "%~dp0..\..\tidy.bat" hello %*
exit /b %errorlevel%
