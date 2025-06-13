#include "Core/Application.h"
#include "Core/Services.h"
#include "Core/Math/Vector2.h"
#include "Core/Logging/Log.h"
#include "Simulation/SimulationWorld.h"
#include "AI/Pathfinding/Pathfinder.h"
#include "Simulation/Material.h" // Required for MATERIAL_EMPTY and MATERIAL_STONE

#include <vector> // Required for std::vector

int main(int argc, char** argv) {
    // Create and initialize BGE::Application
    BGE::Application app;
    app.Initialize();

    // Get SimulationWorld service
    BGE::SimulationWorld* world = BGE::Services::GetWorld();

    if (!world) {
        BGE_LOG_ERROR("Failed to get SimulationWorld service.");
        return -1;
    }

    // Create a 50x50 map
    const int mapWidth = 50;
    const int mapHeight = 50;
    world->CreateWorld(mapWidth, mapHeight);

    // Initialize all tiles to MATERIAL_EMPTY
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            world->SetMaterial(x, y, MATERIAL_EMPTY);
        }
    }

    // Create a 'C' shaped wall using MATERIAL_STONE
    // Vertical part: (25, 10) to (25, 39) (inclusive of 10, exclusive of 40 for height)
    for (int y = 10; y < 40; ++y) {
        world->SetMaterial(25, y, MATERIAL_STONE);
    }
    // Horizontal bottom part: (10, 40) to (24, 40) (inclusive of 10, exclusive of 25 for width)
    // Corrected: y should be 39 if wall is within 0-39 for a height of 40, or use 40 if height is 41.
    // Assuming wall is at y=39 to connect with vertical part ending at y=39.
    // Or, if the vertical part is (25,10) to (25,40), it means y from 10 up to and including 40. Let's adjust.
    // Vertical part: (25, 10) to (25, 40) -> x=25, y from 10 to 40
    for (int y = 10; y <= 40; ++y) {
        if(y >=0 && y < mapHeight) world->SetMaterial(25, y, MATERIAL_STONE);
    }

    // Horizontal bottom part: (10, 40) to (25, 40) -> y=40, x from 10 to 25
    for (int x = 10; x <= 25; ++x) {
         if(x >=0 && x < mapWidth) world->SetMaterial(x, 40, MATERIAL_STONE);
    }

    // Horizontal top part: (10, 10) to (25, 10) -> y=10, x from 10 to 25
    for (int x = 10; x <= 25; ++x) {
         if(x >=0 && x < mapWidth) world->SetMaterial(x, 10, MATERIAL_STONE);
    }


    // Define start and end points
    BGE::Vector2 start(5.0f, 25.0f);
    BGE::Vector2 goal(45.0f, 25.0f);

    // Instantiate Pathfinder
    BGE::Pathfinder pathfinder(world);

    // Call FindPath
    std::vector<BGE::Vector2> path = pathfinder.FindPath(start, goal);

    // Log the path
    BGE_LOG_INFO("Pathfinding Demo:");
    BGE_LOG_INFO("Start: ({}, {})", start.x, start.y);
    BGE_LOG_INFO("Goal: ({}, {})", goal.x, goal.y);

    if (path.empty()) {
        BGE_LOG_INFO("No path found.");
    } else {
        BGE_LOG_INFO("Path found with {} nodes:", path.size());
        for (const auto& p : path) {
            BGE_LOG_INFO("Path node: ({}, {})", p.x, p.y);
        }
    }

    // Clean up BGE::Application
    app.Cleanup();

    return 0;
}
