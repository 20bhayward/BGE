// AI/Pathfinding/Pathfinder.h
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include <queue>
#include <cmath>
#include "Core/Math/Vector2.h"
#include "Simulation/SimulationWorld.h"

namespace BGE {

class Pathfinder {
public:
    Pathfinder(SimulationWorld* world);
    std::vector<BGE::Vector2> FindPath(BGE::Vector2 start, BGE::Vector2 goal);

private:
    SimulationWorld* m_world;

    // Helper function for heuristic (e.g., Manhattan distance)
    int CalculateHeuristic(BGE::Vector2i a, BGE::Vector2i b);

    // Helper function to reconstruct the path
    std::vector<BGE::Vector2> ReconstructPath(struct PathNode* goalNode);
};

} // namespace BGE

#endif // PATHFINDER_H
