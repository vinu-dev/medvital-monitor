@echo off
echo =====================================================
echo   Google Test Setup ^& First Build
echo   (Downloads GTest automatically via CMake)
echo =====================================================
echo.

:: Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found.
    echo.
    echo Please install CMake from: https://cmake.org/download/
    echo Make sure to check "Add CMake to system PATH" during install.
    pause
    exit /b 1
)

:: Check for a C++ compiler (cl.exe for MSVC or g++ for MinGW)
where cl >nul 2>&1
set HAVE_MSVC=%ERRORLEVEL%
where g++ >nul 2>&1
set HAVE_GCC=%ERRORLEVEL%

if %HAVE_MSVC% NEQ 0 if %HAVE_GCC% NEQ 0 (
    echo ERROR: No C++ compiler found.
    echo.
    echo Install one of:
    echo   [Option A] Visual Studio 2022 Community (free):
    echo              https://visualstudio.microsoft.com/vs/community/
    echo              During install select "Desktop development with C++"
    echo.
    echo   [Option B] MinGW-w64 via MSYS2 (free):
    echo              https://www.msys2.org/
    echo              After install run: pacman -S mingw-w64-x86_64-gcc cmake
    pause
    exit /b 1
)

echo [OK] CMake found.
if %HAVE_MSVC% EQU 0 echo [OK] MSVC compiler found.
if %HAVE_GCC% EQU 0  echo [OK] GCC compiler found.

:: Clean any stale build
if exist "build" (
    echo.
    echo Removing old build folder...
    rmdir /s /q build
)

:: Configure — CMake will download Google Test automatically
echo.
echo Configuring project and downloading Google Test...
echo (This may take a minute on first run - downloading from GitHub)
echo.
cmake -S . -B build -DBUILD_TESTS=ON
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed.
    echo Common causes:
    echo   - No internet connection (needed to download Google Test)
    echo   - Git not installed (needed by FetchContent)
    echo   - Compiler not properly set up
    pause
    exit /b 1
)

:: Build everything
echo.
echo Building project and tests...
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed. Check the output above for errors.
    pause
    exit /b 1
)

echo.
echo =====================================================
echo   Setup complete! Google Test is ready.
echo   Run run_tests.bat to execute all tests.
echo =====================================================
pause
