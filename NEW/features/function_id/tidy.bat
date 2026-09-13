@echo off
call "%~dp0..\..\tidy.bat" function_id %*
exit /b %errorlevel%
