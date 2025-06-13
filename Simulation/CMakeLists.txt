add_library(BGESimulation STATIC
    # Core simulation
    SimulationWorld.h
    SimulationWorld.cpp
    CellularAutomata.h
    CellularAutomata.cpp
    
    # Materials system
    Materials/Material.h
    Materials/Material.cpp
    Materials/MaterialSystem.h
    Materials/MaterialSystem.cpp
    Materials/MaterialDatabase.h
    Materials/MaterialDatabase.cpp
    Materials/OpticalProperties.h
    
    # Physics
    Physics/RigidBody.h
    Physics/RigidBody.cpp
    Physics/PhysicsWorld.h
    Physics/PhysicsWorld.cpp
    Physics/Collision.h
    Physics/Collision.cpp
    
    # World management
    World/Chunk.h
    World/Chunk.cpp
    World/ChunkManager.h
    World/ChunkManager.cpp
    World/WorldGenerator.h
    World/WorldGenerator.cpp
)

target_include_directories(BGESimulation PUBLIC . ../ThirdParty)
target_link_libraries(BGESimulation PUBLIC BGECore)

# Optional GPU compute support
if(BGE_USE_VULKAN AND Vulkan_FOUND)
    target_sources(BGESimulation PRIVATE
        Compute/SimulationCompute.h
        Compute/SimulationCompute.cpp
    )
    target_link_libraries(BGESimulation PUBLIC Vulkan::Vulkan)
    target_compile_definitions(BGESimulation PUBLIC BGE_COMPUTE_SUPPORT)
endif()