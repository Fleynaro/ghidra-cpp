@echo off
call "%~dp0..\..\..\build.bat" variadic_function_signature_override %*
exit /b %errorlevel%
