@echo off
setlocal enabledelayedexpansion

echo =====================================================
echo   Patient Vital Signs Monitor -- Documentation
echo   Tool   : Doxygen + Graphviz
echo   Output : docs\html\index.html
echo   Standard: IEC 62304 Class B
echo =====================================================
echo.

:: -------------------------------------------------------
:: Check Doxygen
:: -------------------------------------------------------
where doxygen >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Doxygen not found.
    echo Install it:  winget install --id DimitriVanHeesch.Doxygen
    pause
    exit /b 1
)
for /f "tokens=*" %%v in ('doxygen --version 2^>^&1') do echo [OK] Doxygen %%v

:: -------------------------------------------------------
:: Check Graphviz (optional -- call graphs need dot.exe)
::   Check common install locations across x64 and x86
:: -------------------------------------------------------
set DOT_FOUND=0
for %%P in (
    "C:\Program Files\Graphviz\bin\dot.exe"
    "C:\Program Files (x86)\Graphviz\bin\dot.exe"
    "C:\Program Files\Graphviz2.38\bin\dot.exe"
    "C:\Graphviz\bin\dot.exe"
) do (
    if exist %%P (
        set DOT_FOUND=1
        echo [OK] Graphviz found at %%P
    )
)

where dot >nul 2>&1
if !ERRORLEVEL! EQU 0 (
    set DOT_FOUND=1
    echo [OK] Graphviz dot is on PATH.
)

if !DOT_FOUND! EQU 0 (
    echo [NOTE] Graphviz not found -- call graphs and dependency diagrams
    echo        will be skipped.  Install it with:
    echo          winget install --id Graphviz.Graphviz
    echo        Then re-run this script for full diagram output.
)
echo.

:: -------------------------------------------------------
:: 1. Clean previous output
:: -------------------------------------------------------
echo [1/3] Cleaning previous docs...
if exist "docs" rmdir /s /q "docs"
mkdir docs
echo       Done.

:: -------------------------------------------------------
:: 2. Run Doxygen
:: -------------------------------------------------------
echo [2/3] Running Doxygen...
doxygen Doxyfile
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Doxygen failed. See output above.
    pause
    exit /b 1
)
echo       Done.

:: -------------------------------------------------------
:: 3. Report and open
:: -------------------------------------------------------
echo.
echo [3/3] Documentation generated.
echo.
echo   HTML    : docs\html\index.html   ^<-- open in browser
echo   XML     : docs\xml\
echo   Warnings: docs\doxygen_warnings.log
echo.

if exist "docs\html\index.html" (
    start "" "docs\html\index.html"
) else (
    echo WARNING: docs\html\index.html not found -- check Doxyfile output path.
)

echo.
pause
