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

# Find the required libraries
find_library(GPIOD_LIBRARY NAMES gpiod)
find_library(BCM2835_LIBRARY NAMES bcm2835)
find_library(FFTW_LIBRARY NAMES fftw3)

# If required libraries not found, provide instructions
if(NOT BCM2835_LIBRARY)
  message(STATUS "BCM2835 library not found. Please install it with:")
  message(STATUS "  wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz")
  message(STATUS "  tar zxvf bcm2835-1.71.tar.gz")
  message(STATUS "  cd bcm2835-1.71")
  message(STATUS "  ./configure")
  message(STATUS "  make")
  message(STATUS "  sudo make install")
endif()

if(NOT FFTW_LIBRARY)
  message(STATUS "FFTW library not found. Please install it with:")
  message(STATUS "  sudo apt-get install libfftw3-dev")
endif()

# Define the library sources
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

# Link against the required libraries
target_link_libraries(launch_monitor_lib
    ${GPIOD_LIBRARY}
    ${BCM2835_LIBRARY}
    ${FFTW_LIBRARY}
    m  # Math library
)

# Add test subdirectory
enable_testing()
add_subdirectory(tests)

# Add executable
add_executable(launch_monitor src/main.cpp)
target_link_libraries(launch_monitor launch_monitor_lib)
