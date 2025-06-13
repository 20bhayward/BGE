add_library(BGEAI STATIC
    AISystem.cpp
    AISystem.h
    # TODO: Add these when AI task is completed
    # Pathfinder.cpp
    # Pathfinder.h 
    # BehaviorTree.cpp
    # BehaviorTree.h
)

target_include_directories(BGEAI PUBLIC .)
target_link_libraries(BGEAI PUBLIC BGECore BGESimulation)