@echo off
call "%~dp0..\..\..\build.bat" call_convention_id %*
exit /b %errorlevel%
