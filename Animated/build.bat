@echo off
setlocal

if exist "%~dp0..\w64devkit\bin\g++.exe" (
    set "PATH=%~dp0..\w64devkit\bin;%PATH%"
    "%~dp0..\w64devkit\bin\g++.exe" -std=c++17 -Wall -Wextra -O2 -mwindows "%~dp0main.cpp" -lgdiplus -o "%~dp0dog_mascot.exe"
    exit /b %errorlevel%
)

where cl >nul 2>nul
if %errorlevel%==0 (
    cl /nologo /EHsc /W4 /O2 "%~dp0main.cpp" gdiplus.lib user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /OUT:"%~dp0dog_mascot.exe"
    exit /b %errorlevel%
)

where g++ >nul 2>nul
if %errorlevel%==0 (
    g++ -std=c++17 -Wall -Wextra -O2 -mwindows "%~dp0main.cpp" -lgdiplus -o "%~dp0dog_mascot.exe"
    exit /b %errorlevel%
)

echo Could not find cl.exe or g++ in PATH.
exit /b 1
