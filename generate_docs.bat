@echo off
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
for /f "tokens=*" %%v in ('doxygen --version') do set DOXY_VER=%%v
echo [OK] Doxygen %DOXY_VER%

:: -----------------------------------------------------------------------
:: 2. Check Graphviz (dot) — required for call graphs and diagrams
:: -----------------------------------------------------------------------
where dot >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [WARNING] Graphviz (dot) not found — diagrams will be skipped.
    echo.
    echo Install via winget:
    echo   winget install --id Graphviz.Graphviz
    echo.
    echo After install, re-run this script to get full diagram output.
    echo Continuing without diagrams...
    echo.
    :: Temporarily disable dot in a patched Doxyfile
    powershell -Command "(Get-Content Doxyfile) -replace 'HAVE_DOT\s*=\s*YES','HAVE_DOT = NO' | Set-Content Doxyfile.tmp"
    set DOXYFILE=Doxyfile.tmp
) else (
    for /f "tokens=*" %%v in ('dot -V 2^>^&1') do set DOT_VER=%%v
    echo [OK] Graphviz: %DOT_VER%
    set DOXYFILE=Doxyfile
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
:: 4. Run Doxygen
:: -----------------------------------------------------------------------
echo.
echo [2/3] Running Doxygen...
doxygen %DOXYFILE% 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Doxygen failed. Check output above.
    if exist Doxyfile.tmp del /q Doxyfile.tmp
    pause
    exit /b 1
)

:: Clean up temp file if created
if exist Doxyfile.tmp del /q Doxyfile.tmp

:: -----------------------------------------------------------------------
:: 5. Summary + open browser
:: -----------------------------------------------------------------------
echo.
echo [3/3] Documentation generated.
echo.
echo =====================================================
echo   OUTPUT
echo =====================================================
echo.
echo   HTML (browsable)  : docs\html\index.html
echo   XML  (machine)    : docs\xml\
echo   Warnings log      : docs\doxygen_warnings.log
echo.
echo   Sections generated:
echo     - Module list with file descriptions
echo     - Data structure reference (VitalSigns, Alert, PatientRecord)
echo     - Function reference with parameters, return values, pre/postconditions
echo     - Requirement traceability tags (REQ-VIT, REQ-ALT, REQ-PAT, REQ-INT)
echo     - Call graphs and caller graphs per function
echo     - Include dependency graphs
echo     - Annotated source browser
echo.

if exist "docs\doxygen_warnings.log" (
    for %%A in ("docs\doxygen_warnings.log") do set WSIZE=%%~zA
    if !WSIZE! GTR 0 (
        echo   [NOTE] Warnings found in docs\doxygen_warnings.log
        echo          Review these for IEC 62304 documentation completeness.
    ) else (
        echo   [OK] No documentation warnings.
    )
)

echo.
echo Opening documentation in browser...
start "" "docs\html\index.html"

echo.
pause
