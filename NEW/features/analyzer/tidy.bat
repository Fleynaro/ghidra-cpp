@echo off
call "%~dp0..\..\tidy.bat" analyzer %*
exit /b %errorlevel%
