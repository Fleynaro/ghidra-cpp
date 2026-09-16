@echo off
call "%~dp0..\..\..\build.bat" pdb_universal %*
exit /b %errorlevel%
