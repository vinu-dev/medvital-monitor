@echo off
setlocal enabledelayedexpansion

echo =====================================================
echo   Generate Architecture + Design Documentation
echo   Tool   : Doxygen + Graphviz
echo   Output : docs\html\index.html
echo   Standard: IEC 62304 Class B
echo =====================================================
echo.

:: -----------------------------------------------------------------------
:: 1. Check Doxygen
:: -----------------------------------------------------------------------
where doxygen >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Doxygen not found.
    echo.
    echo Install via winget:
    echo   winget install --id DimitriVanHeesch.Doxygen
    echo.
    echo Or download from: https://www.doxygen.nl/download.html
    pause
    exit /b 1
)
echo [OK] Doxygen found.

:: -----------------------------------------------------------------------
:: 2. Check Graphviz (dot) -- required for call graphs and diagrams
:: -----------------------------------------------------------------------
set USE_DOT=YES
where dot >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [WARNING] Graphviz (dot) not found -- diagrams will be skipped.
    echo.
    echo Install via winget:
    echo   winget install --id Graphviz.Graphviz
    echo.
    echo After install, re-run this script to get full diagram output.
    echo Continuing without diagrams...
    echo.
    set USE_DOT=NO
) else (
    echo [OK] Graphviz found.
)

:: -----------------------------------------------------------------------
:: 3. Clean previous output
:: -----------------------------------------------------------------------
echo.
echo [1/3] Cleaning previous documentation...
if exist "docs" rmdir /s /q "docs"
mkdir docs
echo       Done.

:: -----------------------------------------------------------------------
:: 4. Build a temporary Doxyfile with correct HAVE_DOT setting
:: -----------------------------------------------------------------------
if "!USE_DOT!"=="NO" (
    powershell -Command "(Get-Content 'Doxyfile') -replace 'HAVE_DOT\s*=\s*YES', 'HAVE_DOT = NO' | Set-Content 'Doxyfile.tmp'"
    set DOXYFILE=Doxyfile.tmp
) else (
    set DOXYFILE=Doxyfile
)

:: -----------------------------------------------------------------------
:: 5. Run Doxygen
:: -----------------------------------------------------------------------
echo.
echo [2/3] Running Doxygen...
doxygen %DOXYFILE%
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Doxygen failed. Check output above.
    if exist "Doxyfile.tmp" del /q "Doxyfile.tmp"
    pause
    exit /b 1
)

if exist "Doxyfile.tmp" del /q "Doxyfile.tmp"

:: -----------------------------------------------------------------------
:: 6. Summary and open browser
:: -----------------------------------------------------------------------
echo.
echo [3/3] Documentation generated successfully.
echo.
echo =====================================================
echo   OUTPUT
echo =====================================================
echo.
echo   HTML (browsable)  : docs\html\index.html
echo   XML  (machine)    : docs\xml\
echo   Warnings log      : docs\doxygen_warnings.log
echo.
echo   Includes:
echo     - Module and file descriptions
echo     - Data structure reference (VitalSigns, Alert, PatientRecord)
echo     - Function reference with parameters, pre/postconditions, req tags
echo     - Requirement traceability (REQ-VIT, REQ-ALT, REQ-PAT, REQ-INT)
echo     - Annotated source browser

if "!USE_DOT!"=="YES" (
    echo     - Call graphs, caller graphs, include dependency graphs
    echo     - UML-style collaboration diagrams
)

echo.
if exist "docs\doxygen_warnings.log" (
    echo   Warnings log: docs\doxygen_warnings.log
    echo   Review for IEC 62304 documentation completeness.
)

echo.
echo Opening documentation in browser...
start "" "docs\html\index.html"

echo.
pause
endlocal
