@echo off
call "%~dp0..\..\..\build.bat" pdb_msdia %*
exit /b %errorlevel%
