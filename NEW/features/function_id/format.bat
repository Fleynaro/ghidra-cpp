@echo off
call "%~dp0..\..\format.bat" function_id %*
exit /b %errorlevel%
