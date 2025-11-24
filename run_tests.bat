@echo off
REM Build and run tests script for Windows

echo ========================================
echo Building Matching Engine Tests
echo ========================================

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Build the project
cd build
cmake .. -DBUILD_TESTS=ON
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    exit /b %ERRORLEVEL%
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo Running Tests
echo ========================================
echo.

REM Run the tests
Release\matching_engine_tests.exe
set TEST_RESULT=%ERRORLEVEL%

cd ..

echo.
if %TEST_RESULT% EQU 0 (
    echo ========================================
    echo All tests passed!
    echo ========================================
) else (
    echo ========================================
    echo Some tests failed!
    echo ========================================
)

exit /b %TEST_RESULT%

