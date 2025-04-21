# Enable testing
enable_testing()

# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add test executable
add_executable(launch_monitor_tests
    logger_test.cpp
    radar_test.cpp
    camera_test.cpp
    trigger_test.cpp
    main_test.cpp
)

# Link test executable with GTest and your project libraries
target_link_libraries(launch_monitor_tests
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
    launch_monitor_lib
)

# Include directories
target_include_directories(launch_monitor_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Add test to CTest
add_test(NAME launch_monitor_tests COMMAND launch_monitor_tests)