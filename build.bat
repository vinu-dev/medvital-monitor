@echo off
echo ================================
echo   Building Patient Monitor
echo ================================

:: Add MinGW to PATH if present (needed for GCC + mingw32-make)
if exist "C:\MinGW\bin\gcc.exe" (
    set "PATH=C:\MinGW\bin;%PATH%"
)

:: Check if cmake is available
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

:: Detect if build needs a clean reconfigure
:: (required when switching GTest version or generator)
set NEEDS_CLEAN=0
if not exist "build" set NEEDS_CLEAN=1
if exist "build\_CMakeCache.txt.tmp" set NEEDS_CLEAN=1

:: Check that the cached generator matches MinGW Makefiles
if exist "build\CMakeCache.txt" (
    findstr /c:"CMAKE_GENERATOR:INTERNAL=MinGW Makefiles" build\CMakeCache.txt >nul 2>&1
    if %ERRORLEVEL% NEQ 0 set NEEDS_CLEAN=1
)

if %NEEDS_CLEAN% EQU 1 (
    echo Cleaning build directory for fresh configure...
    rmdir /s /q build 2>nul
)

:: Run CMake configure
echo.
echo Configuring...
cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_C_COMPILER=gcc ^
      -DCMAKE_CXX_COMPILER=g++
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

:: Run the GUI executable
echo.
echo ================================
echo   Launching Patient Monitor GUI
echo ================================
if exist "build\patient_monitor_gui.exe" (
    start "" "build\patient_monitor_gui.exe"
) else if exist "build\Debug\patient_monitor_gui.exe" (
    start "" "build\Debug\patient_monitor_gui.exe"
) else (
    echo WARNING: GUI executable not found.
)

echo.
echo Done!
pause
