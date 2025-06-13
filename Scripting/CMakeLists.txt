# BGE Scripting System
# Currently minimal implementation

add_library(BGEScripting STATIC
    # Placeholder files - to be implemented
    ScriptingSystem.h
    ScriptingSystem.cpp
)

target_include_directories(BGEScripting PUBLIC .)
target_link_libraries(BGEScripting PUBLIC BGECore)

# TODO: Add Lua or other scripting libraries when implemented