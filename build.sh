#!/bin/bash
#Build script for Linux/Unix/MacOS

set -e

echo "========================================="
echo "Building Matching Engine"
echo "========================================="

# Create build directory
mkdir -p build
cd build

# Configure and build
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "========================================="
echo "Build completed successfully!"
echo "========================================="
echo "Executables are in the build/ directory:"
echo "  - matching_server"
echo "  - matching_client"
echo ""
echo "To run the server: ./build/matching_server"
echo "To run the client: ./build/matching_client"
echo "========================================="

