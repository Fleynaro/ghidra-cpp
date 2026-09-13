@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a deterministic, CRT-free MSVC x64 PE for AggressiveInstructionFinderAnalyzer.

where cl >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: MSVC cl.exe is not available and vswhere.exe was not found.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if not defined VSINSTALL (
        echo ERROR: An MSVC installation with x64 tools was not found.
        exit /b 1
    )
    call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64
    if errorlevel 1 exit /b 1
)

cl /nologo /c test_aggressive_instruction_finder.cpp /Fo:test_aggressive_instruction_finder.obj ^
    /O1 /Ob0 /Oi- /Oy- /GS- /GR- /EHs-c- /GL- /Gy- /Gw- /Zc:inline- ^
    /W4 /WX /Brepro
if errorlevel 1 exit /b 1

link /nologo test_aggressive_instruction_finder.obj /OUT:test_aggressive_instruction_finder.exe ^
    /SUBSYSTEM:CONSOLE /ENTRY:aggressive_instruction_finder_entry /NODEFAULTLIB ^
    /OPT:NOREF /OPT:NOICF /INCREMENTAL:NO /DEBUG:NONE /BREPRO ^
    /EXPORT:aggressive_instruction_finder_entry /EXPORT:aggressive_candidate ^
    /EXPORT:seed_00 /EXPORT:seed_01 /EXPORT:seed_02 /EXPORT:seed_03 /EXPORT:seed_04 ^
    /EXPORT:seed_05 /EXPORT:seed_06 /EXPORT:seed_07 /EXPORT:seed_08 /EXPORT:seed_09 ^
    /EXPORT:seed_10 /EXPORT:seed_11 /EXPORT:seed_12 /EXPORT:seed_13 /EXPORT:seed_14 ^
    /EXPORT:seed_15 /EXPORT:seed_16 /EXPORT:seed_17 /EXPORT:seed_18 /EXPORT:seed_19 ^
    /EXPORT:seed_20 /EXPORT:seed_21 /EXPORT:seed_22 /EXPORT:seed_23
if errorlevel 1 exit /b 1

del /q test_aggressive_instruction_finder.obj >nul 2>&1
del /q test_aggressive_instruction_finder.lib test_aggressive_instruction_finder.exp >nul 2>&1
echo Built test_aggressive_instruction_finder.exe
exit /b 0
