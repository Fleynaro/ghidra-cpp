@echo off
call "%~dp0..\..\..\build.bat" embedded_media %*
exit /b %errorlevel%
