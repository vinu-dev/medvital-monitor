@echo off
echo ================================
echo   Building MyApp with CMake
echo ================================

:: Check if cmake is available
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

:: Create build folder if it doesn't exist
if not exist "build" (
    echo Creating build folder...
    mkdir build
)

:: Run CMake configure
echo.
echo Configuring...
cmake -S . -B build
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed.
    pause
    exit /b 1
)

:: Build the project
echo.
echo Building...
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

:: Run the executable
echo.
echo ================================
echo   Running myapp
echo ================================
build\Debug\myapp.exe 2>nul || build\myapp.exe

echo.
echo Done!
pause
