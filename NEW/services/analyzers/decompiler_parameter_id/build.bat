@echo off
call "%~dp0..\..\..\build.bat" decompiler_parameter_id %*
exit /b %errorlevel%
