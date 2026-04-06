@echo off
echo =====================================================
echo   Install Code Coverage Tools
echo   Tool: OpenCppCoverage (MSVC / Windows)
echo   Required for: IEC 62304 / FDA SW Validation
echo =====================================================
echo.

:: Check if already installed
where OpenCppCoverage >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] OpenCppCoverage is already installed.
    OpenCppCoverage --version
    echo.
    echo Nothing to do. Run run_coverage.bat to generate reports.
    pause
    exit /b 0
)

echo OpenCppCoverage not found. Installing via winget...
echo.

:: Install via winget
winget install --id OpenCppCoverage.OpenCppCoverage --silent --accept-package-agreements --accept-source-agreements
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo WARNING: winget install may have had issues.
    echo If OpenCppCoverage was not installed, download it manually:
    echo   https://github.com/OpenCppCoverage/OpenCppCoverage/releases
    echo   Download: OpenCppCoverage-x86-0.9.9.0.exe and run it.
    pause
    exit /b 1
)

echo.
echo =====================================================
echo   Refreshing PATH so OpenCppCoverage is available
echo =====================================================

:: Reload PATH from registry so new install is visible in this session
for /f "tokens=2*" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PATH 2^>nul') do set "SYSTEM_PATH=%%B"
for /f "tokens=2*" %%A in ('reg query "HKCU\Environment" /v PATH 2^>nul') do set "USER_PATH=%%B"
set "PATH=%SYSTEM_PATH%;%USER_PATH%"

where OpenCppCoverage >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] OpenCppCoverage installed successfully.
    OpenCppCoverage --version
) else (
    echo [NOTE] OpenCppCoverage installed but not yet in PATH.
    echo        Please close this window, open a new terminal, then run run_coverage.bat
)

echo.
echo =====================================================
echo   Done. Run run_coverage.bat to generate reports.
echo =====================================================
pause
