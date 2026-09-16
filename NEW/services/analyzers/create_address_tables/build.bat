@echo off
call "%~dp0..\..\..\build.bat" create_address_tables %*
exit /b %errorlevel%
