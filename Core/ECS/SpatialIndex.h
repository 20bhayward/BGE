#pragma once

#include "EntityID.h"
#include "Components/HierarchyComponent.h"
#include "../Logger.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace BGE {

// Axis-aligned bounding box
struct AABB {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    
    AABB() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}
    
    AABB(float x, float y, float z, float radius)
        : minX(x - radius), minY(y - radius), minZ(z - radius)
        , maxX(x + radius), maxY(y + radius), maxZ(z + radius) {}
    
    bool Intersects(const AABB& other) const {
        return (minX <= other.maxX && maxX >= other.minX) &&
               (minY <= other.maxY && maxY >= other.minY) &&
               (minZ <= other.maxZ && maxZ >= other.minZ);
    }
    
    bool Contains(float x, float y, float z) const {
        return x >= minX && x <= maxX &&
               y >= minY && y <= maxY &&
               z >= minZ && z <= maxZ;
    }
    
    float GetCenterX() const { return (minX + maxX) * 0.5f; }
    float GetCenterY() const { return (minY + maxY) * 0.5f; }
    float GetCenterZ() const { return (minZ + maxZ) * 0.5f; }
};

// Spatial hash grid for efficient spatial queries
class SpatialHashGrid {
public:
    static constexpr float DEFAULT_CELL_SIZE = 10.0f;
    
    explicit SpatialHashGrid(float cellSize = DEFAULT_CELL_SIZE) 
        : m_cellSize(cellSize), m_invCellSize(1.0f / cellSize) {}
    
    // Insert entity at position
    void Insert(EntityID entity, float x, float y, float z) {
        int64_t cellKey = GetCellKey(x, y, z);
        m_cells[cellKey].push_back(entity);
        m_entityCells[entity] = cellKey;
    }
    
    // Update entity position
    void Update(EntityID entity, float x, float y, float z) {
        // Remove from old cell
        auto it = m_entityCells.find(entity);
        if (it != m_entityCells.end()) {
            int64_t oldCell = it->second;
            auto& oldCellEntities = m_cells[oldCell];
            oldCellEntities.erase(
                std::remove(oldCellEntities.begin(), oldCellEntities.end(), entity),
                oldCellEntities.end()
            );
            
            if (oldCellEntities.empty()) {
                m_cells.erase(oldCell);
            }
        }
        
        // Insert into new cell
        Insert(entity, x, y, z);
    }
    
    // Remove entity
    void Remove(EntityID entity) {
        auto it = m_entityCells.find(entity);
        if (it != m_entityCells.end()) {
            int64_t cellKey = it->second;
            auto& cellEntities = m_cells[cellKey];
            cellEntities.erase(
                std::remove(cellEntities.begin(), cellEntities.end(), entity),
                cellEntities.end()
            );
            
            if (cellEntities.empty()) {
                m_cells.erase(cellKey);
            }
            
            m_entityCells.erase(it);
        }
    }
    
    // Query entities within radius
    std::vector<EntityID> QueryRadius(float x, float y, float z, float radius) const {
        std::vector<EntityID> result;
        
        // Calculate cell range to check
        int minCellX = static_cast<int>((x - radius) * m_invCellSize);
        int maxCellX = static_cast<int>((x + radius) * m_invCellSize);
        int minCellY = static_cast<int>((y - radius) * m_invCellSize);
        int maxCellY = static_cast<int>((y + radius) * m_invCellSize);
        int minCellZ = static_cast<int>((z - radius) * m_invCellSize);
        int maxCellZ = static_cast<int>((z + radius) * m_invCellSize);
        
        float radiusSq = radius * radius;
        
        // Check all cells in range
        for (int cx = minCellX; cx <= maxCellX; ++cx) {
            for (int cy = minCellY; cy <= maxCellY; ++cy) {
                for (int cz = minCellZ; cz <= maxCellZ; ++cz) {
                    int64_t cellKey = MakeCellKey(cx, cy, cz);
                    auto cellIt = m_cells.find(cellKey);
                    if (cellIt != m_cells.end()) {
                        // Add entities within radius
                        for (EntityID entity : cellIt->second) {
                            // Would need position lookup here - simplified for now
                            result.push_back(entity);
                        }
                    }
                }
            }
        }
        
        return result;
    }
    
    // Query entities within AABB
    std::vector<EntityID> QueryAABB(const AABB& bounds) const {
        std::vector<EntityID> result;
        
        // Calculate cell range
        int minCellX = static_cast<int>(bounds.minX * m_invCellSize);
        int maxCellX = static_cast<int>(bounds.maxX * m_invCellSize);
        int minCellY = static_cast<int>(bounds.minY * m_invCellSize);
        int maxCellY = static_cast<int>(bounds.maxY * m_invCellSize);
        int minCellZ = static_cast<int>(bounds.minZ * m_invCellSize);
        int maxCellZ = static_cast<int>(bounds.maxZ * m_invCellSize);
        
        // Check all cells in range
        for (int cx = minCellX; cx <= maxCellX; ++cx) {
            for (int cy = minCellY; cy <= maxCellY; ++cy) {
                for (int cz = minCellZ; cz <= maxCellZ; ++cz) {
                    int64_t cellKey = MakeCellKey(cx, cy, cz);
                    auto cellIt = m_cells.find(cellKey);
                    if (cellIt != m_cells.end()) {
                        for (EntityID entity : cellIt->second) {
                            result.push_back(entity);
                        }
                    }
                }
            }
        }
        
        return result;
    }
    
    // Clear all data
    void Clear() {
        m_cells.clear();
        m_entityCells.clear();
    }
    
    // Get statistics
    size_t GetCellCount() const { return m_cells.size(); }
    size_t GetEntityCount() const { return m_entityCells.size(); }
    
private:
    int64_t GetCellKey(float x, float y, float z) const {
        int cellX = static_cast<int>(x * m_invCellSize);
        int cellY = static_cast<int>(y * m_invCellSize);
        int cellZ = static_cast<int>(z * m_invCellSize);
        return MakeCellKey(cellX, cellY, cellZ);
    }
    
    int64_t MakeCellKey(int x, int y, int z) const {
        // Pack 3D coordinates into 64-bit key
        return (static_cast<int64_t>(x) << 42) | 
               (static_cast<int64_t>(y) << 21) | 
               static_cast<int64_t>(z);
    }
    
    float m_cellSize;
    float m_invCellSize;
    std::unordered_map<int64_t, std::vector<EntityID>> m_cells;
    std::unordered_map<EntityID, int64_t, EntityIDHash> m_entityCells;
};

// Octree node for hierarchical spatial indexing
template<size_t MaxEntitiesPerNode = 8>
class OctreeNode {
public:
    OctreeNode(const AABB& bounds) : m_bounds(bounds), m_isLeaf(true) {}
    
    void Insert(EntityID entity, float x, float y, float z) {
        if (m_isLeaf) {
            m_entities.push_back({entity, x, y, z});
            
            if (m_entities.size() > MaxEntitiesPerNode && m_bounds.maxX - m_bounds.minX > 1.0f) {
                Subdivide();
            }
        } else {
            // Insert into appropriate child
            int childIndex = GetChildIndex(x, y, z);
            if (childIndex >= 0 && m_children[childIndex]) {
                m_children[childIndex]->Insert(entity, x, y, z);
            }
        }
    }
    
    void Query(const AABB& queryBounds, std::vector<EntityID>& results) const {
        if (!m_bounds.Intersects(queryBounds)) {
            return;
        }
        
        if (m_isLeaf) {
            for (const auto& entry : m_entities) {
                if (queryBounds.Contains(entry.x, entry.y, entry.z)) {
                    results.push_back(entry.entity);
                }
            }
        } else {
            for (const auto& child : m_children) {
                if (child) {
                    child->Query(queryBounds, results);
                }
            }
        }
    }
    
private:
    struct EntityEntry {
        EntityID entity;
        float x, y, z;
    };
    
    void Subdivide() {
        float cx = m_bounds.GetCenterX();
        float cy = m_bounds.GetCenterY();
        float cz = m_bounds.GetCenterZ();
        
        // Create 8 children
        for (int i = 0; i < 8; ++i) {
            AABB childBounds;
            childBounds.minX = (i & 1) ? cx : m_bounds.minX;
            childBounds.maxX = (i & 1) ? m_bounds.maxX : cx;
            childBounds.minY = (i & 2) ? cy : m_bounds.minY;
            childBounds.maxY = (i & 2) ? m_bounds.maxY : cy;
            childBounds.minZ = (i & 4) ? cz : m_bounds.minZ;
            childBounds.maxZ = (i & 4) ? m_bounds.maxZ : cz;
            
            m_children[i] = std::make_unique<OctreeNode>(childBounds);
        }
        
        // Move entities to children
        for (const auto& entry : m_entities) {
            int childIndex = GetChildIndex(entry.x, entry.y, entry.z);
            if (childIndex >= 0) {
                m_children[childIndex]->Insert(entry.entity, entry.x, entry.y, entry.z);
            }
        }
        
        m_entities.clear();
        m_isLeaf = false;
    }
    
    int GetChildIndex(float x, float y, float z) const {
        float cx = m_bounds.GetCenterX();
        float cy = m_bounds.GetCenterY();
        float cz = m_bounds.GetCenterZ();
        
        int index = 0;
        if (x >= cx) index |= 1;
        if (y >= cy) index |= 2;
        if (z >= cz) index |= 4;
        
        return index;
    }
    
    AABB m_bounds;
    bool m_isLeaf;
    std::vector<EntityEntry> m_entities;
    std::unique_ptr<OctreeNode> m_children[8];
};

} // namespace BGE