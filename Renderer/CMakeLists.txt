cmake_minimum_required(VERSION 3.10)
project(Renderer)

set(RENDERER_SOURCES
    Renderer.cpp
    PixelCamera.cpp
    ParticleSystem.cpp
)

# Assuming BGE is the main target executable or library
# Add sources to the parent target (e.g., BGECore or whatever your main library/executable is)
target_sources(BGE PUBLIC ${RENDERER_SOURCES})

# Include directories (if necessary for headers in this module)
target_include_directories(BGE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
