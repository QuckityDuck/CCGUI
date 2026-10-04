@echo off
setlocal

cd /d "%~dp0"

if not exist build mkdir build

where g++ >nul 2>nul
if errorlevel 1 (
    echo.
    echo CGUI+ ERROR: g++ was not found.
    echo Install MinGW-w64/MSYS2 and make sure g++ is on PATH.
    echo.
    exit /b 1
)

echo Building CGUI+...

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude -c src\cgui.cpp -o build\cgui.o
if errorlevel 1 goto :failed

where ar >nul 2>nul
if errorlevel 1 (
    echo.
    echo CGUI+ ERROR: ar was not found.
    echo Your MinGW installation should include ar.exe.
    echo.
    exit /b 1
)

ar rcs build\libcgui.a build\cgui.o
if errorlevel 1 goto :failed

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude examples\main.cpp build\libcgui.a -o build\cgui_example.exe -luser32 -lgdi32 -lgdiplus
if errorlevel 1 goto :failed

echo.
echo CGUI+ built successfully.
echo Library: build\libcgui.a
echo Example: build\cgui_example.exe
exit /b 0

:failed
echo.
echo CGUI+ build failed.
exit /b 1
