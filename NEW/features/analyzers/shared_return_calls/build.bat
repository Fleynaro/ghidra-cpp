@echo off
call "%~dp0..\..\..\build.bat" shared_return_calls %*
exit /b %errorlevel%
