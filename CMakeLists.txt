cmake_minimum_required(VERSION 3.10)
project(SandSimEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files for the executable
add_executable(SandSim
    main.cpp
    Engine.cpp
    Window.cpp
    Grid.cpp
    Renderer.cpp
    MaterialRegistry.cpp
    UIManager.cpp
)

# If headers were in an "include" directory, we would add:
# target_include_directories(SandSim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# For now, all .h files are in the root with .cpp files, so CMake usually finds them.

# Optional: Add compiler warnings (good practice)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Optional: Link against libraries if needed in the future (e.g., for graphics, windowing)
# target_link_libraries(SandSim PRIVATE some_library)
