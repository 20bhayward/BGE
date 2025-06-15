add_library(BGERenderer STATIC
    Renderer.cpp
    Renderer.h
    PixelCamera.cpp
    PixelCamera.h
    ParticleSystem.cpp
    ParticleSystem.h
    PostProcessor.cpp
    PostProcessor.h
    Texture.h
)

target_include_directories(BGERenderer PUBLIC .)
target_link_libraries(BGERenderer PUBLIC BGECore)