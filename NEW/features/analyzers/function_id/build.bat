@echo off
call "%~dp0..\..\..\build.bat" function_id_analyzer %*
exit /b %errorlevel%
