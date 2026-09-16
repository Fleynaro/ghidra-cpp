@echo off
call "%~dp0..\..\..\build.bat" condense_filler_bytes %*
exit /b %errorlevel%
