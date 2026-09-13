@echo off
call "%~dp0..\..\..\build.bat" scalar_operand_references %*
exit /b %errorlevel%
