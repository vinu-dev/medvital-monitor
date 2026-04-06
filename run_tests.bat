@echo off
echo =====================================================
echo   Running All Tests (Unit + Integration)
echo =====================================================
echo.

:: Make sure the build folder exists
if not exist "build" (
    echo ERROR: Build folder not found.
    echo Please run setup_gtest.bat first.
    pause
    exit /b 1
)

:: Rebuild to pick up any code changes
echo Rebuilding...
cmake --build build --config Debug >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Rebuild had issues. Running last successful build.
)

echo.
echo -----------------------------------------------------
echo  UNIT TESTS
echo -----------------------------------------------------
ctest --test-dir build -R UnitTests --output-on-failure -C Debug
set UNIT_RESULT=%ERRORLEVEL%

echo.
echo -----------------------------------------------------
echo  INTEGRATION TESTS
echo -----------------------------------------------------
ctest --test-dir build -R IntegrationTests --output-on-failure -C Debug
set INT_RESULT=%ERRORLEVEL%

echo.
echo -----------------------------------------------------
echo  TEST REPORT (XML) saved to:
echo    build\results_unit.xml
echo    build\results_integration.xml
echo -----------------------------------------------------
echo.

if %UNIT_RESULT% EQU 0 if %INT_RESULT% EQU 0 (
    echo [PASS] All tests passed.
) else (
    echo [FAIL] One or more tests failed. See output above.
)

echo.
pause
