# ECS Unit Tests

# Find GTest
find_package(GTest REQUIRED)

# Test sources
set(ECS_TEST_SOURCES
    ThreadSafetyTests.cpp
    EntityManagerTests.cpp
    ComponentStorageTests.cpp
    QueryTests.cpp
    PerformanceTests.cpp
)

# Create test executable
add_executable(ECSTests ${ECS_TEST_SOURCES})

# Link dependencies
target_link_libraries(ECSTests
    PRIVATE
        BGECore
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

# Include directories
target_include_directories(ECSTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

# Add to CTest
include(GoogleTest)
gtest_discover_tests(ECSTests)

# Enable thread sanitizer for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ECSTests PRIVATE -fsanitize=thread)
    target_link_options(ECSTests PRIVATE -fsanitize=thread)
endif()