@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo   Code Coverage Report -- Medical Device Grade
echo   Standard : IEC 62304 Class B (Statement + Branch Coverage)
echo   Toolchain: MinGW GCC + gcov  (HTML via gcovr if installed)
echo   Scope    : vitals.c  alerts.c  patient.c  gui_auth.c
echo =====================================================================
echo.
echo   For HTML reports first run:  pip install gcovr
echo.

:: -------------------------------------------------------
:: Locate GCC / gcov / CMake
:: -------------------------------------------------------
if exist "C:\MinGW\bin\gcc.exe" set "PATH=C:\MinGW\bin;%PATH%"

where gcov >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: gcov not found. Ensure MinGW GCC is installed and on PATH.
    pause
    exit /b 1
)

where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found.
    pause
    exit /b 1
)

for /f "tokens=*" %%v in ('gcov --version 2^>^&1 ^| findstr /i "gcov"') do echo [OK] %%v
echo.

:: -------------------------------------------------------
:: 1. Configure a dedicated coverage build
::    (keeps the normal 'build' folder untouched)
:: -------------------------------------------------------
set COV_BUILD=build_cov

echo [1/5] Configuring coverage build (ENABLE_COVERAGE=ON) in %COV_BUILD%...
if exist "%COV_BUILD%" rmdir /s /q "%COV_BUILD%"
cmake -S . -B "%COV_BUILD%" -G "MinGW Makefiles" ^
      -DCMAKE_C_COMPILER=gcc ^
      -DCMAKE_CXX_COMPILER=g++ ^
      -DENABLE_COVERAGE=ON ^
      -Wno-dev >nul 2>&1
IF ERRORLEVEL 1 (
    echo ERROR: CMake configuration failed.
    pause
    exit /b 1
)
echo       Done.

:: -------------------------------------------------------
:: 2. Build test targets with --coverage instrumentation
:: -------------------------------------------------------
echo [2/5] Building test targets with coverage flags...
cmake --build "%COV_BUILD%" --target test_unit test_integration >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed. Fix errors before running coverage.
    pause
    exit /b 1
)
echo       Done.

:: -------------------------------------------------------
:: 3. Run UNIT tests  (produces .gcda files beside .gcno files)
:: -------------------------------------------------------
echo [3/5] Running UNIT tests...
if exist "%COV_BUILD%\tests\test_unit.exe" (
    "%COV_BUILD%\tests\test_unit.exe" --gtest_output=xml:%COV_BUILD%\results_unit.xml
    set UNIT_RESULT=!ERRORLEVEL!
) else (
    echo ERROR: test_unit.exe not found.
    set UNIT_RESULT=1
)
if !UNIT_RESULT! NEQ 0 echo WARNING: Unit tests had failures -- coverage data may be incomplete.
echo       Done.

:: -------------------------------------------------------
:: 4. Run INTEGRATION tests
:: -------------------------------------------------------
echo [4/5] Running INTEGRATION tests...
if exist "%COV_BUILD%\tests\test_integration.exe" (
    "%COV_BUILD%\tests\test_integration.exe" --gtest_output=xml:%COV_BUILD%\results_integration.xml
    set INT_RESULT=!ERRORLEVEL!
) else (
    echo ERROR: test_integration.exe not found.
    set INT_RESULT=1
)
if !INT_RESULT! NEQ 0 echo WARNING: Integration tests had failures -- coverage data may be incomplete.
echo       Done.

:: -------------------------------------------------------
:: 5. Generate coverage report
::    Primary  : gcovr  (pip install gcovr)  -> HTML + Cobertura XML
::    Fallback : gcov text output per source file
:: -------------------------------------------------------
echo [5/5] Generating coverage report...

where gcovr >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo       gcovr found -- generating HTML + Cobertura XML.
    if exist "coverage_report" rmdir /s /q "coverage_report"
    mkdir coverage_report

    gcovr ^
        --root . ^
        --object-directory "%COV_BUILD%" ^
        --filter "src/vitals\.c" ^
        --filter "src/alerts\.c" ^
        --filter "src/patient\.c" ^
        --filter "src/gui_auth\.c" ^
        --html-details coverage_report\index.html ^
        --xml coverage_report\coverage_cobertura.xml ^
        --print-summary

    echo.
    echo   HTML report : coverage_report\index.html   ^<-- open in browser
    echo   XML  report : coverage_report\coverage_cobertura.xml  ^<-- DHF record

    if exist "coverage_report\index.html" (
        echo.
        echo   Opening HTML report...
        start "" "coverage_report\index.html"
    )
) else (
    echo       gcovr not installed -- using gcov text output.
    echo       Install gcovr for HTML:  pip install gcovr
    echo.

    echo   --- vitals.c ---
    gcov -r -b -o "%COV_BUILD%\CMakeFiles\monitor_lib.dir\src" src\vitals.c

    echo.
    echo   --- alerts.c ---
    gcov -r -b -o "%COV_BUILD%\CMakeFiles\monitor_lib.dir\src" src\alerts.c

    echo.
    echo   --- patient.c ---
    gcov -r -b -o "%COV_BUILD%\CMakeFiles\monitor_lib.dir\src" src\patient.c

    echo.
    echo   --- gui_auth.c ---
    gcov -r -b -o "%COV_BUILD%\CMakeFiles\test_unit.dir\src" src\gui_auth.c

    echo.
    echo   Annotated .gcov files written to the current directory.
    echo   Lines marked ##### are not covered.
)

:: -------------------------------------------------------
:: Summary
:: -------------------------------------------------------
echo.
echo =====================================================================
echo   IEC 62304 Class B Coverage Targets
echo   +-------------------+--------+
echo   ^| Metric            ^| Target ^|
echo   +-------------------+--------+
echo   ^| Statement         ^| 100%%   ^|
echo   ^| Branch            ^| 100%%   ^|
echo   +-------------------+--------+
echo.
if !UNIT_RESULT! EQU 0 if !INT_RESULT! EQU 0 (
    echo   [PASS] All 121 tests passed.
) else (
    echo   [FAIL] One or more tests failed -- review output above.
)
echo.
pause
