@echo off
call "%~dp0..\..\..\build.bat" subroutine_references %*
exit /b %errorlevel%
