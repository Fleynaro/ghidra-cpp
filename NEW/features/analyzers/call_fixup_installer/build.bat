@echo off
call "%~dp0..\..\..\build.bat" call_fixup_installer %*
exit /b %errorlevel%
