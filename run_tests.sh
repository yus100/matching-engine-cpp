#!/bin/bash
# Build and run tests script for Linux/Mac

set -e

echo "========================================"
echo "Building Matching Engine Tests"
echo "========================================"

# Create build directory if it doesn't exist
mkdir -p build

# Build the project
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --config Release

echo ""
echo "========================================"
echo "Running Tests"
echo "========================================"
echo ""

# Run the tests
./matching_engine_tests

TEST_RESULT=$?

cd ..

echo ""
if [ $TEST_RESULT -eq 0 ]; then
    echo "========================================"
    echo "All tests passed!"
    echo "========================================"
else
    echo "========================================"
    echo "Some tests failed!"
    echo "========================================"
fi

exit $TEST_RESULT

