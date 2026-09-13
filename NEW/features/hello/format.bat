@echo off
call "%~dp0..\..\format.bat" hello %*
exit /b %errorlevel%
