cmake_minimum_required(VERSION 3.10)
project(GolfLaunchMonitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers
include_directories(include)

# Collect all sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(launch_monitor ${SOURCES})
