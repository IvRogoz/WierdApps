@echo off
setlocal

if exist "%~dp0..\w64devkit\bin\gcc.exe" (
    set "PATH=%~dp0..\w64devkit\bin;%PATH%"
    "%~dp0..\w64devkit\bin\gcc.exe" -std=c11 -Wall -Wextra -O2 -mwindows "%~dp0main.c" -lgdi32 -o "%~dp0imagedshape.exe"
    exit /b %errorlevel%
)

where cl >nul 2>nul
if %errorlevel%==0 (
    cl /nologo /W4 /O2 "%~dp0main.c" user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /OUT:"%~dp0imagedshape.exe"
    exit /b %errorlevel%
)

where gcc >nul 2>nul
if %errorlevel%==0 (
    gcc -std=c11 -Wall -Wextra -O2 -mwindows "%~dp0main.c" -lgdi32 -o "%~dp0imagedshape.exe"
    exit /b %errorlevel%
)

echo Could not find cl.exe or gcc in PATH.
exit /b 1
