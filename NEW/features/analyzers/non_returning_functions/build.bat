@echo off
call "%~dp0..\..\..\build.bat" non_returning_functions %*
exit /b %errorlevel%
