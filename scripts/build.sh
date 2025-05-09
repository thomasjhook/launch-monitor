#!/bin/bash

if [ ! -d "build" ]; then
  echo "Creating build directory..."
  mkdir build
fi

# Navigate to build directory
cd build

# Run CMake to configure the project
echo "Running cmake..."
cmake ..

# Build the project using make
echo "Building the project..."
make

echo "Build completed! Run the program with ./build/launch_monitor"