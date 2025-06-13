# AI/CMakeLists.txt
add_library(BGEAI STATIC
    # AI class files will be added here
    Pathfinding/Pathfinder.cpp
    Pathfinding/Pathfinder.h
    BehaviorTree/BTNode.h
    BehaviorTree/BehaviorTree.cpp
    BehaviorTree/BehaviorTree.h
    BehaviorTree/CompositeNode.cpp
    BehaviorTree/CompositeNode.h
    BehaviorTree/SequenceNode.cpp
    BehaviorTree/SequenceNode.h
    BehaviorTree/SelectorNode.cpp
    BehaviorTree/SelectorNode.h
    # ... other files
)

target_include_directories(BGEAI PUBLIC .)
target_link_libraries(BGEAI PUBLIC BGECore BGESimulation)
