@echo off
setlocal enabledelayedexpansion

echo =====================================================
echo   Patient Vital Signs Monitor -- Build
echo   Toolchain : MinGW GCC + CMake (MinGW Makefiles)
echo   Tests     : Included  (BUILD_TESTS=ON)
echo =====================================================
echo.

:: -------------------------------------------------------
:: 1. Locate GCC -- try default MinGW path, then system PATH
:: -------------------------------------------------------
if exist "C:\MinGW\bin\gcc.exe" set "PATH=C:\MinGW\bin;%PATH%"

where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GCC not found.
    echo.
    echo Install MinGW:  https://sourceforge.net/projects/mingw/
    echo   During setup select: mingw32-gcc-g++, mingw32-make
    echo   Then add C:\MinGW\bin to your system PATH.
    echo.
    echo Or install MSYS2:  winget install --id MSYS2.MSYS2
    echo   Then in MSYS2 MinGW64 shell run:
    echo   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
    pause
    exit /b 1
)

where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found.
    echo Install it:  winget install Kitware.CMake
    pause
    exit /b 1
)

for /f "tokens=*" %%v in ('gcc --version 2^>^&1 ^| findstr /i "gcc"') do echo [OK] %%v
echo [OK] CMake found.
echo.

:: -------------------------------------------------------
:: 2. Detect whether existing build needs a clean reconfigure
::    Required when switching generator or compiler toolchain
:: -------------------------------------------------------
set NEEDS_CLEAN=0
if not exist "build" set NEEDS_CLEAN=1

if exist "build\CMakeCache.txt" (
    findstr /c:"CMAKE_GENERATOR:INTERNAL=MinGW Makefiles" "build\CMakeCache.txt" >nul 2>&1
    if !ERRORLEVEL! NEQ 0 set NEEDS_CLEAN=1
)

if !NEEDS_CLEAN! EQU 1 (
    echo Cleaning build directory for fresh configure...
    if exist "build" rmdir /s /q build
    echo.
)

:: -------------------------------------------------------
:: 3. Configure
:: -------------------------------------------------------
echo Configuring...
cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_C_COMPILER=gcc ^
      -DCMAKE_CXX_COMPILER=g++ ^
      -DBUILD_TESTS=ON ^
      -Wno-dev
set CMAKE_RESULT=%ERRORLEVEL%
if %CMAKE_RESULT% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed.
    echo Common causes:
    echo   - No internet connection  (FetchContent downloads Google Test)
    echo   - Git not installed       winget install Git.Git
    echo   - Compiler not on PATH
    pause
    exit /b 1
)

:: -------------------------------------------------------
:: 4. Build all targets
:: -------------------------------------------------------
echo.
echo Building...
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed. See errors above.
    pause
    exit /b 1
)

echo.
echo =====================================================
echo   Build complete.
echo.
echo   GUI app  : build\patient_monitor_gui.exe
echo   Console  : build\patient_monitor.exe
echo   Tests    : build\tests\test_unit.exe
echo              build\tests\test_integration.exe
echo.
echo   Run tests    : run_tests.bat
echo   Run coverage : run_coverage.bat
echo   Generate docs: generate_docs.bat
echo =====================================================
echo.

:: -------------------------------------------------------
:: 5. Launch GUI
:: -------------------------------------------------------
if exist "build\patient_monitor_gui.exe" (
    echo Launching Patient Monitor GUI...
    start "" "build\patient_monitor_gui.exe"
) else (
    echo WARNING: patient_monitor_gui.exe not found.
)

echo.
pause
