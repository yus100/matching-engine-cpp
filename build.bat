@echo off
REM Build script for Windows

echo =========================================
echo Building Matching Engine
echo =========================================

REM Create build directory
if not exist build mkdir build
cd build

REM Configure and build
echo Configuring CMake...
cmake ..

echo Building...
cmake --build . --config Release

echo.
echo =========================================
echo Build completed successfully!
echo =========================================
echo Executables are in the build\Release\ directory:
echo   - matching_server.exe
echo   - matching_client.exe
echo.
echo To run the server: build\Release\matching_server.exe
echo To run the client: build\Release\matching_client.exe
echo =========================================

cd ..

