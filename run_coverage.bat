@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo   Code Coverage Report — Medical Device Grade
echo   Standard : IEC 62304 Class B (Statement + Branch Coverage)
echo   Tool     : OpenCppCoverage
echo   Scope    : src\utils.c  (production code only)
echo =====================================================================
echo.

:: -----------------------------------------------------------------------
:: 0. Check prerequisites
:: -----------------------------------------------------------------------
where OpenCppCoverage >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: OpenCppCoverage not found.
    echo Run install_coverage_tools.bat first, then re-open this terminal.
    pause
    exit /b 1
)

if not exist "build\tests\Debug\test_unit.exe" (
    echo ERROR: test_unit.exe not found. Run setup_gtest.bat first.
    pause
    exit /b 1
)

if not exist "build\tests\Debug\test_integration.exe" (
    echo ERROR: test_integration.exe not found. Run setup_gtest.bat first.
    pause
    exit /b 1
)

:: -----------------------------------------------------------------------
:: 1. Rebuild (pick up latest code changes)
:: -----------------------------------------------------------------------
echo [1/5] Rebuilding tests...
cmake --build build --config Debug >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed. Fix errors before running coverage.
    pause
    exit /b 1
)
echo       Done.

:: -----------------------------------------------------------------------
:: 2. Clean old coverage outputs
:: -----------------------------------------------------------------------
echo [2/5] Cleaning old coverage data...
if exist "coverage_report"      rmdir /s /q "coverage_report"
if exist "coverage_combined"    rmdir /s /q "coverage_combined"
if exist "coverage_unit.cov"    del /q "coverage_unit.cov"
if exist "coverage_integ.cov"   del /q "coverage_integ.cov"
if exist "coverage_report.xml"  del /q "coverage_report.xml"
echo       Done.

:: -----------------------------------------------------------------------
:: 3. Run UNIT tests with coverage instrumentation
::    --sources     : only instrument production source, not gtest internals
::    --excluded_sources : exclude test files themselves from the report
::    --export_type binary : save raw coverage for merging
:: -----------------------------------------------------------------------
echo [3/5] Running UNIT tests with coverage...
OpenCppCoverage ^
    --sources "%CD%\src" ^
    --sources "%CD%\include" ^
    --excluded_sources "%CD%\tests" ^
    --excluded_sources "%CD%\build\_deps" ^
    --export_type binary:coverage_unit.cov ^
    --quiet ^
    -- "build\tests\Debug\test_unit.exe" --gtest_output=xml:build\results_unit.xml

if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Unit tests reported failures. Coverage data may be incomplete.
    echo          Review build\results_unit.xml for details.
)
echo       Done.

:: -----------------------------------------------------------------------
:: 4. Run INTEGRATION tests with coverage, merged with unit coverage
::    --input_coverage : merge the unit coverage collected in step 3
:: -----------------------------------------------------------------------
echo [4/5] Running INTEGRATION tests with coverage (merged)...
OpenCppCoverage ^
    --sources "%CD%\src" ^
    --sources "%CD%\include" ^
    --excluded_sources "%CD%\tests" ^
    --excluded_sources "%CD%\build\_deps" ^
    --input_coverage coverage_unit.cov ^
    --export_type html:coverage_combined ^
    --export_type cobertura:coverage_report.xml ^
    --quiet ^
    -- "build\tests\Debug\test_integration.exe" --gtest_output=xml:build\results_integration.xml

if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Integration tests reported failures. Coverage may be incomplete.
    echo          Review build\results_integration.xml for details.
)
echo       Done.

:: -----------------------------------------------------------------------
:: 5. Print summary and open report
:: -----------------------------------------------------------------------
echo [5/5] Finalising reports...
echo.
echo =====================================================================
echo   COVERAGE REPORTS GENERATED
echo =====================================================================
echo.
echo   [HTML]  coverage_combined\index.html   ^<-- open in browser
echo   [XML]   coverage_report.xml            ^<-- Cobertura format (DHF record)
echo   [XML]   build\results_unit.xml         ^<-- Unit test results
echo   [XML]   build\results_integration.xml  ^<-- Integration test results
echo.
echo   IEC 62304 Coverage Targets:
echo   +-----------------+------------+-------------------------------+
echo   ^| Coverage Type   ^| Class B    ^| Class C (safety-critical)    ^|
echo   +-----------------+------------+-------------------------------+
echo   ^| Statement       ^| 100%%       ^| 100%%                         ^|
echo   ^| Branch          ^| 100%%       ^| 100%%                         ^|
echo   ^| MC/DC           ^| N/A        ^| 100%% (needs specialist tool)  ^|
echo   +-----------------+------------+-------------------------------+
echo.
echo   NOTE: MC/DC (IEC 62304 Class C) requires a specialist tool
echo         such as BullseyeCoverage or VectorCAST.
echo.

:: Open the HTML report automatically
if exist "coverage_combined\index.html" (
    echo Opening HTML report in default browser...
    start "" "coverage_combined\index.html"
) else (
    echo WARNING: HTML report not found. Check OpenCppCoverage output above.
)

echo.
pause
