# BGE Renderer Module

add_library(BGERenderer STATIC
    Renderer.cpp
    Renderer.h
    Texture.h # Texture.h is header only but good to list if it's specific to this module
)

target_include_directories(BGERenderer PUBLIC .)

# Renderer might depend on Core for things like Window, Logger, ServiceLocator etc.
# Check if BGECore is the correct target name from Core/CMakeLists.txt
target_link_libraries(BGERenderer PUBLIC BGECore)

# If Renderer uses OpenGL/Vulkan directly, add those dependencies here.
# For now, it's a placeholder, so BGECore might be sufficient.
