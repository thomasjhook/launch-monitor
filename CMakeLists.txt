cmake_minimum_required(VERSION 3.14)
project(LaunchMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(launch_monitor_lib
    src/logger.cpp
    src/radar.cpp
    src/camera.cpp
    src/trigger.cpp
)

# Define include directories for the library
target_include_directories(launch_monitor_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Add test subdirectory
add_subdirectory(tests)

# Add executable
add_executable(launch_monitor src/main.cpp)
target_link_libraries(launch_monitor launch_monitor_lib)
