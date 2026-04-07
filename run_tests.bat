@echo off
echo =====================================================
echo   Running All Tests (Unit + Integration)
echo =====================================================
echo.

:: Add MinGW to PATH if present
if exist "C:\MinGW\bin\gcc.exe" (
    set "PATH=C:\MinGW\bin;%PATH%"
)

:: Make sure the build folder exists
if not exist "build" (
    echo ERROR: Build folder not found. Run build.bat first.
    pause
    exit /b 1
)

:: Rebuild tests only (fast)
echo Rebuilding...
cmake --build build --target test_unit --target test_integration >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Rebuild had issues — running last successful build.
)

echo.
echo -----------------------------------------------------
echo  UNIT TESTS
echo -----------------------------------------------------
if exist "build\tests\test_unit.exe" (
    build\tests\test_unit.exe --gtest_output=xml:build\results_unit.xml
    set UNIT_RESULT=%ERRORLEVEL%
) else if exist "build\test_unit.exe" (
    build\test_unit.exe --gtest_output=xml:build\results_unit.xml
    set UNIT_RESULT=%ERRORLEVEL%
) else (
    echo ERROR: test_unit.exe not found. Run build.bat first.
    set UNIT_RESULT=1
)

echo.
echo -----------------------------------------------------
echo  INTEGRATION TESTS
echo -----------------------------------------------------
if exist "build\tests\test_integration.exe" (
    build\tests\test_integration.exe --gtest_output=xml:build\results_integration.xml
    set INT_RESULT=%ERRORLEVEL%
) else if exist "build\test_integration.exe" (
    build\test_integration.exe --gtest_output=xml:build\results_integration.xml
    set INT_RESULT=%ERRORLEVEL%
) else (
    echo ERROR: test_integration.exe not found. Run build.bat first.
    set INT_RESULT=1
)

echo.
echo -----------------------------------------------------
echo  TEST REPORT (XML):
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
