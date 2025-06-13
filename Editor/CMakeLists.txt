# BGE Editor
# Currently minimal implementation

add_library(BGEEditor STATIC
    # Placeholder files - to be implemented
    EditorSystem.h
    EditorSystem.cpp
)

target_include_directories(BGEEditor PUBLIC .)
target_link_libraries(BGEEditor PUBLIC BGECore BGERenderer BGESimulation)

# TODO: Add editor UI libraries (ImGui, etc.) when implemented