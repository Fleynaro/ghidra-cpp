@echo off
call "%~dp0..\..\..\build.bat" aggressive_instruction_finder %*
exit /b %errorlevel%
