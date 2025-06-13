# BGE Audio System
# Currently minimal implementation

add_library(BGEAudio STATIC
    # Placeholder files - to be implemented
    AudioSystem.h
    AudioSystem.cpp
)

target_include_directories(BGEAudio PUBLIC .)
target_link_libraries(BGEAudio PUBLIC BGECore)

# TODO: Add audio libraries (OpenAL, FMOD, etc.) when implemented