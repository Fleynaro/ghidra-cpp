@echo off
call "%~dp0..\..\..\build.bat" disassemble_entry_points %*
exit /b %errorlevel%
