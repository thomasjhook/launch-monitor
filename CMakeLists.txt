cmake_minimum_required(VERSION 3.14)
project(LaunchMonitor)

# Include FetchContent module
include(FetchContent)

# Download and configure GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0 
)
# Make GoogleTest available for the project
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the libgpiod package
find_library(GPIOD_LIBRARY NAMES gpiod)
find_library(GPIODCXX_LIBRARY NAMES gpiodcxx)

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

# Link libgpiod library
target_link_libraries(launch_monitor_lib 
    ${GPIOD_LIBRARY}
    ${GPIODCXX_LIBRARY}
)

# Add test subdirectory
enable_testing()
add_subdirectory(tests)

# Add executable
add_executable(launch_monitor src/main.cpp)
target_link_libraries(launch_monitor launch_monitor_lib)
