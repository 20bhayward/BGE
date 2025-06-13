# BGE Runtime System
# Currently minimal implementation

add_library(BGERuntime STATIC
    # Placeholder files - to be implemented
    RuntimeSystem.h
    RuntimeSystem.cpp
)

target_include_directories(BGERuntime PUBLIC .)
target_link_libraries(BGERuntime PUBLIC BGECore)

# TODO: Add runtime functionality when implemented