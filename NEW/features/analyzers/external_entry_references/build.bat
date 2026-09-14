@echo off
call "%~dp0..\..\..\build.bat" external_entry_references %*
exit /b %errorlevel%
