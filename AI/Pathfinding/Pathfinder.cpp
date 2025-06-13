// AI/Pathfinding/Pathfinder.cpp
#include "AI/Pathfinding/Pathfinder.h"
#include "Core/Services.h"
#include "Core/Math/Vector2i.h" // Required for Vector2i
#include <unordered_set> // For closed list optimization
#include <algorithm> // For std::reverse

// Define BGE::Vector2i hash for unordered_set
namespace std {
    template <>
    struct hash<BGE::Vector2i> {
        std::size_t operator()(const BGE::Vector2i& v) const {
            // A simple hash function for Vector2i
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
}


namespace BGE {

struct PathNode {
    BGE::Vector2i position;
    int gCost; // Cost from start to this node
    int hCost; // Heuristic cost from this node to goal
    PathNode* parent;

    int fCost() const { return gCost + hCost; }

    // For priority queue (min-heap)
    bool operator>(const PathNode& other) const {
        if (fCost() == other.fCost()) {
            return hCost > other.hCost; // Tie-breaking: prefer lower hCost
        }
        return fCost() > other.fCost();
    }

    // Equality for searching in lists
    bool operator==(const PathNode& other) const {
        return position == other.position;
    }
};

Pathfinder::Pathfinder(SimulationWorld* world) : m_world(world) {
    if (!m_world) {
        // Fallback if world is not provided through constructor, though Services::GetWorld() is preferred
        m_world = Services::GetWorld();
    }
}

// Helper function for heuristic (Manhattan distance)
int Pathfinder::CalculateHeuristic(BGE::Vector2i a, BGE::Vector2i b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

// Helper function to reconstruct the path
std::vector<BGE::Vector2> Pathfinder::ReconstructPath(PathNode* goalNode) {
    std::vector<BGE::Vector2> path;
    PathNode* currentNode = goalNode;
    while (currentNode != nullptr) {
        path.push_back(BGE::Vector2(static_cast<float>(currentNode->position.x), static_cast<float>(currentNode->position.y)));
        currentNode = currentNode->parent;
    }
    std::reverse(path.begin(), path.end()); // Path is reconstructed backwards
    return path;
}

std::vector<BGE::Vector2> Pathfinder::FindPath(BGE::Vector2 start, BGE::Vector2 goal) {
    if (!m_world) {
        // Attempt to get world via Services if not set or null
        m_world = Services::GetWorld();
        if (!m_world) {
            // LOG_ERROR("Pathfinder: SimulationWorld is not available.");
            return {}; // Return empty path if world is not available
        }
    }

    BGE::Vector2i startInt(static_cast<int>(start.x), static_cast<int>(start.y));
    BGE::Vector2i goalInt(static_cast<int>(goal.x), static_cast<int>(goal.y));

    // Check if start or goal is outside bounds or not empty
    if (m_world->GetMaterial(startInt.x, startInt.y) != MATERIAL_EMPTY ||
        m_world->GetMaterial(goalInt.x, goalInt.y) != MATERIAL_EMPTY) {
        // LOG_WARNING("Pathfinder: Start or goal position is obstructed or out of bounds.");
        return {};
    }

    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openList;
    std::unordered_set<BGE::Vector2i> closedSet; // Positions of nodes in the closed list
    std::vector<PathNode*> allNodes; // To manage memory of PathNode objects

    PathNode* startNode = new PathNode{startInt, 0, CalculateHeuristic(startInt, goalInt), nullptr};
    openList.push(*startNode);
    allNodes.push_back(startNode);

    // Define potential neighbors (4-directional movement)
    const BGE::Vector2i neighbors[] = {
        {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        // Add {1,1}, {1,-1}, {-1,1}, {-1,-1} for 8-directional
    };

    while (!openList.empty()) {
        PathNode currentPathNode = openList.top();
        openList.pop();

        // Because we store PathNode directly in priority_queue, we need to find its pointer from allNodes
        // This is not ideal. A better approach would be to store PathNode* in the priority queue
        // and provide a custom comparator, or use a map to quickly find nodes.
        // For simplicity in this context, we'll proceed, but be aware of performance implications.
        // A proper A* would typically use a structure that allows efficient update of node costs.

        // Find the pointer to the current node in allNodes to update its parent if necessary
        // This linear search is inefficient.
        PathNode* currentNodePtr = nullptr;
        for(PathNode* node : allNodes) {
            if(node->position == currentPathNode.position) {
                currentNodePtr = node;
                break;
            }
        }
        // If somehow not found (should not happen if logic is correct)
        if (!currentNodePtr) continue;


        if (currentNodePtr->position == goalInt) {
            std::vector<BGE::Vector2> resultPath = ReconstructPath(currentNodePtr);
            // Cleanup allocated nodes
            for (PathNode* node : allNodes) delete node;
            return resultPath;
        }

        if (closedSet.count(currentNodePtr->position)) {
            continue; // Already processed
        }
        closedSet.insert(currentNodePtr->position);


        for (const auto& move : neighbors) {
            BGE::Vector2i neighborPos = currentNodePtr->position + move;

            // Check bounds and if passable
            if (neighborPos.x < 0 || neighborPos.x >= m_world->GetWidth() ||
                neighborPos.y < 0 || neighborPos.y >= m_world->GetHeight() ||
                m_world->GetMaterial(neighborPos.x, neighborPos.y) != MATERIAL_EMPTY) {
                continue;
            }

            if (closedSet.count(neighborPos)) {
                continue; // Already evaluated
            }

            int tentativeGCost = currentNodePtr->gCost + 1; // Assuming cost of 1 to move to neighbor

            // Check if neighbor is in openList or if this path is better
            // This part is tricky with std::priority_queue as it doesn't support decrease-key easily.
            // A common workaround is to add multiple entries for the same node if a shorter path is found.
            // The check `if (closedSet.count(neighborPos))` handles not reprocessing,
            // and the priority queue ensures we always pick the one with the lowest fCost first.

            bool inOpenList = false;
            // A simple way to check if a node is "in" openList (not directly, but if we've considered it)
            // is by searching allNodes. This is inefficient.
            // A more robust A* would use a map from Vector2i to PathNode* or PathNode itself
            // to quickly check existence and update costs.

            PathNode* existingNeighborNode = nullptr;
            for(PathNode* node : allNodes) {
                if(node->position == neighborPos) {
                    existingNeighborNode = node;
                    break;
                }
            }

            if (existingNeighborNode == nullptr) { // Not encountered before
                PathNode* neighborNode = new PathNode{
                    neighborPos,
                    tentativeGCost,
                    CalculateHeuristic(neighborPos, goalInt),
                    currentNodePtr
                };
                openList.push(*neighborNode);
                allNodes.push_back(neighborNode);
            } else if (tentativeGCost < existingNeighborNode->gCost) {
                // Found a better path to this existing node
                existingNeighborNode->gCost = tentativeGCost;
                existingNeighborNode->parent = currentNodePtr;
                // Re-add to priority queue with updated cost.
                // This creates duplicates but works because we process the lowest fCost first.
                // The closedSet check prevents reprocessing of the worse-cost duplicate.
                openList.push(*existingNeighborNode);
            }
        }
    }

    // Cleanup allocated nodes if path not found
    for (PathNode* node : allNodes) delete node;
    return {}; // Return empty path if goal not reached
}

} // namespace BGE
