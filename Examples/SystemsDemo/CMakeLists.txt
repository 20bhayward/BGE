cmake_minimum_required(VERSION 3.10)

# SystemsDemo executable
add_executable(SystemsDemo main.cpp)

# Link with Core library
target_link_libraries(SystemsDemo PRIVATE BGECore)

# Include directories
target_include_directories(SystemsDemo PRIVATE ${CMAKE_SOURCE_DIR})

# Set properties
set_target_properties(SystemsDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)