@echo off
setlocal

if exist "%~dp0w64devkit\bin\gcc.exe" (
    set "PATH=%~dp0w64devkit\bin;%PATH%"
    "%~dp0w64devkit\bin\gcc.exe" -std=c11 -Wall -Wextra -O2 -mwindows "%~dp0main.c" -lgdi32 -lm -o "%~dp0weirdshape.exe"
    exit /b %errorlevel%
)

where cl >nul 2>nul
if %errorlevel%==0 (
    cl /nologo /W4 /O2 main.c user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /OUT:weirdshape.exe
    exit /b %errorlevel%
)

where gcc >nul 2>nul
if %errorlevel%==0 (
    gcc -std=c11 -Wall -Wextra -O2 -mwindows main.c -lgdi32 -lm -o weirdshape.exe
    exit /b %errorlevel%
)

echo Could not find cl.exe or gcc in PATH.
echo Open a Developer Command Prompt for Visual Studio, or install MinGW-w64.
exit /b 1
