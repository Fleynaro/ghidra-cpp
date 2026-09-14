@echo off
call "%~dp0..\..\..\build.bat" apply_data_archives %*
exit /b %errorlevel%
