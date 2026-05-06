@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo   Patient Vital Signs Monitor -- Windows Installer Builder
echo   Tool   : Inno Setup 6
echo   Output : dist\PatientMonitorSetup-2.7.0.exe
echo =====================================================================
echo.

:: -------------------------------------------------------
:: Locate GCC / CMake
:: -------------------------------------------------------
if exist "C:\MinGW\bin\gcc.exe" set "PATH=C:\MinGW\bin;%PATH%"

where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GCC not found. Ensure MinGW is installed and on PATH.
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

:: Fix Git dubious ownership (network drives / non-ownership filesystems)
git config --global --add safe.directory * >nul 2>&1

:: -------------------------------------------------------
:: 1. Build the GUI application (tests OFF for release build)
:: -------------------------------------------------------
echo [1/3] Building patient_monitor_gui.exe...

set NEEDS_CLEAN=0
if not exist "build" set NEEDS_CLEAN=1
if exist "build\CMakeCache.txt" (
    findstr /c:"CMAKE_GENERATOR:INTERNAL=MinGW Makefiles" "build\CMakeCache.txt" >nul 2>&1
    if !ERRORLEVEL! NEQ 0 set NEEDS_CLEAN=1
)
if !NEEDS_CLEAN! EQU 1 (
    if exist "build" rmdir /s /q build
)

cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_C_COMPILER=gcc ^
      -DCMAKE_CXX_COMPILER=g++ ^
      -DBUILD_TESTS=OFF ^
      -Wno-dev >nul 2>&1
IF ERRORLEVEL 1 (
    echo ERROR: CMake configuration failed.
    pause
    exit /b 1
)

cmake --build build --target patient_monitor_gui >nul 2>&1
IF ERRORLEVEL 1 (
    echo ERROR: Build failed. See errors above.
    pause
    exit /b 1
)

if not exist "build\patient_monitor_gui.exe" (
    echo ERROR: patient_monitor_gui.exe not produced.
    pause
    exit /b 1
)
echo       Done  --  build\patient_monitor_gui.exe

:: -------------------------------------------------------
:: 2. Locate Inno Setup compiler (ISCC.exe)
:: -------------------------------------------------------
echo.
echo [2/3] Locating Inno Setup 6...

set ISCC=
where ISCC >nul 2>&1
if %ERRORLEVEL% EQU 0 for /f "tokens=*" %%I in ('where ISCC') do set ISCC=%%I

if "!ISCC!"=="" (
    for %%P in (
        "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
        "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
        "%ProgramFiles%\Inno Setup 6\ISCC.exe"
        "%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe"
        "%ProgramFiles%\Inno Setup 5\ISCC.exe"
    ) do (
        if exist %%P set ISCC=%%~P
    )
)

if "!ISCC!"=="" (
    echo.
    echo ERROR: Inno Setup 6 not found.
    echo.
    echo Install it with:
    echo   winget install --id JRSoftware.InnoSetup
    echo.
    echo Then re-run this script.
    pause
    exit /b 1
)
echo       Found: !ISCC!

:: -------------------------------------------------------
:: 3. Compile the installer
:: -------------------------------------------------------
echo.
echo [3/3] Compiling installer...
if exist "dist" rmdir /s /q dist
mkdir dist

"!ISCC!" installer.iss
IF ERRORLEVEL 1 (
    echo.
    echo ERROR: Inno Setup compilation failed.  Check installer.iss.
    pause
    exit /b 1
)

echo.
echo =====================================================================
echo   Installer built successfully.
echo.
echo   File    : dist\PatientMonitorSetup-2.7.0.exe
echo   Install : double-click to run the wizard
echo   Remove  : Settings ^> Apps ^> Patient Vital Signs Monitor ^> Uninstall
echo            (or Control Panel ^> Programs ^> Uninstall a program)
echo =====================================================================
echo.
pause
