@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Toolchain, triplet, overlay: see CMakePresets.json (preset vs2026).
cmake --preset vs2026
if %errorlevel% neq 0 (
  echo CMake configure failed!
  exit /b %errorlevel%
)

cmake --build --preset debug
if %errorlevel% neq 0 (
  echo Build failed!
  exit /b %errorlevel%
)

echo Build successful! Output: _build\bin\Debug\main.exe
