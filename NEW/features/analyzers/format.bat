@echo off
call "%~dp0..\..\format.bat" analyzer %*
exit /b %errorlevel%
