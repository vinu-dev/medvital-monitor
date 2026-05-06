@echo off
setlocal enabledelayedexpansion

echo =====================================================
echo   Patient Vital Signs Monitor -- Test Runner
echo   312 tests: Unit (298) + Integration (14)
echo   Standard : IEC 62304 Class B
echo =====================================================
echo.

:: -------------------------------------------------------
:: Locate GCC / CMake
:: -------------------------------------------------------
if exist "C:\MinGW\bin\gcc.exe" set "PATH=C:\MinGW\bin;%PATH%"

where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Run build.bat first.
    pause
    exit /b 1
)

:: -------------------------------------------------------
:: Ensure the build folder exists
:: -------------------------------------------------------
if not exist "build" (
    echo ERROR: Build folder not found. Run build.bat first.
    pause
    exit /b 1
)

set "TEST_TEMP_DIR=%CD%\build\test_tmp"
if not exist "%TEST_TEMP_DIR%" mkdir "%TEST_TEMP_DIR%"
set "TMP=%TEST_TEMP_DIR%"
set "TEMP=%TEST_TEMP_DIR%"
set "TMPDIR=%TEST_TEMP_DIR%"

:: -------------------------------------------------------
:: Rebuild test targets (fast -- only recompiles changed files)
:: -------------------------------------------------------
echo Rebuilding test targets...
cmake --build build --target test_unit test_integration >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Rebuild had issues -- running last successful build.
)
echo.

:: -------------------------------------------------------
:: UNIT TESTS
:: -------------------------------------------------------
echo -----------------------------------------------------
echo  UNIT TESTS
echo -----------------------------------------------------
if exist "build\tests\test_unit.exe" (
    build\tests\test_unit.exe --gtest_output=xml:build\results_unit.xml
    set UNIT_RESULT=!ERRORLEVEL!
) else (
    echo ERROR: build\tests\test_unit.exe not found. Run build.bat first.
    set UNIT_RESULT=1
)

echo.

:: -------------------------------------------------------
:: INTEGRATION TESTS
:: -------------------------------------------------------
echo -----------------------------------------------------
echo  INTEGRATION TESTS
echo -----------------------------------------------------
if exist "build\tests\test_integration.exe" (
    build\tests\test_integration.exe --gtest_output=xml:build\results_integration.xml
    set INT_RESULT=!ERRORLEVEL!
) else (
    echo ERROR: build\tests\test_integration.exe not found. Run build.bat first.
    set INT_RESULT=1
)

echo.
echo -----------------------------------------------------
echo  REPORTS (JUnit XML for DHF / audit trail)
echo    build\results_unit.xml
echo    build\results_integration.xml
echo -----------------------------------------------------
echo.

:: -------------------------------------------------------
:: Summary and exit code
:: -------------------------------------------------------
if !UNIT_RESULT! EQU 0 if !INT_RESULT! EQU 0 (
    echo [PASS] All 312 tests passed.
    echo.
    pause
    exit /b 0
) else (
    echo [FAIL] One or more tests failed. See output above.
    echo.
    pause
    exit /b 1
)
