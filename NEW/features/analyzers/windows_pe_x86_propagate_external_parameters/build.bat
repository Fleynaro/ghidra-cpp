@echo off
call "%~dp0..\..\..\build.bat" windows_pe_x86_propagate_external_parameters %*
exit /b %errorlevel%
