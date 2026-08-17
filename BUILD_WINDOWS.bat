@echo off
setlocal
cd /d "%~dp0"

echo ============================================================
echo BBK Black-19 - Windows x64 VST3 build
echo ============================================================
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake was not found in PATH.
    echo Install Visual Studio 2022 with "Desktop development with C++"
    echo and "C++ CMake tools for Windows", then run this from a
    echo Developer Command Prompt for VS 2022.
    echo.
    pause
    exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :failed

cmake --build build --config Release --target BBKBlack19_VST3
if errorlevel 1 goto :failed

echo.
echo BUILD COMPLETE.
echo Expected plugin location:
echo   build\BBKBlack19_artefacts\Release\VST3\BBK Black-19.vst3
echo.
pause
exit /b 0

:failed
echo.
echo BUILD FAILED. Scroll up and copy the complete error output.
echo.
pause
exit /b 1
