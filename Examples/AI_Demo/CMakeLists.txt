cmake_minimum_required(VERSION 3.10)
project(AI_Demo)

# Define the source files for the executable
set(SOURCES
    main.cpp
)

# Add the executable target
add_executable(AI_Demo ${SOURCES})

# Link against the engine libraries
target_link_libraries(AI_Demo PRIVATE BGECore BGESimulation BGEAI)

# Ensure the executable is output to the main build directory
set_target_properties(AI_Demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
