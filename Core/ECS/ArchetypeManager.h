#pragma once

#include "Archetype.h"
#include "EntityID.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace BGE {

class ArchetypeManager {
public:
    ArchetypeManager() {
        // Create empty archetype
        ComponentMask emptyMask;
        std::vector<ComponentTypeID> emptyTypes;
        m_archetypes.push_back(std::make_unique<Archetype>(emptyMask, emptyTypes));
        
        // Initialize edges for empty archetype
        UpdateArchetypeEdges(0);
    }
    
    // Find or create archetype with given component mask
    uint32_t GetOrCreateArchetype(const ComponentMask& mask, const std::vector<ComponentTypeID>& types) {
        // Check if archetype already exists
        for (size_t i = 0; i < m_archetypes.size(); ++i) {
            if (m_archetypes[i]->GetMask() == mask) {
                return static_cast<uint32_t>(i);
            }
        }
        
        // Create new archetype
        uint32_t index = static_cast<uint32_t>(m_archetypes.size());
        m_archetypes.push_back(std::make_unique<Archetype>(mask, types));
        
        // Update edges for faster transitions
        UpdateArchetypeEdges(index);
        
        return index;
    }
    
    // Get archetype by index
    Archetype* GetArchetype(uint32_t index) {
        return index < m_archetypes.size() ? m_archetypes[index].get() : nullptr;
    }
    
    const Archetype* GetArchetype(uint32_t index) const {
        return index < m_archetypes.size() ? m_archetypes[index].get() : nullptr;
    }
    
    // Find archetype when adding a component
    uint32_t GetArchetypeAfterAdd(uint32_t currentArchetype, ComponentTypeID componentType) {
        // Ensure edges vector is large enough for current archetype
        if (currentArchetype >= m_archetypeEdges.size()) {
            UpdateArchetypeEdges(currentArchetype);
        }
        
        auto it = m_archetypeEdges[currentArchetype].find(componentType);
        if (it != m_archetypeEdges[currentArchetype].end()) {
            return it->second.add;
        }
        
        // Calculate new archetype
        Archetype* current = GetArchetype(currentArchetype);
        if (!current || current->HasComponent(componentType)) {
            return currentArchetype; // Already has component
        }
        
        ComponentMask newMask = current->GetMask();
        newMask.set(componentType);
        
        std::vector<ComponentTypeID> newTypes = current->GetComponentTypes();
        newTypes.push_back(componentType);
        
        uint32_t newArchetype = GetOrCreateArchetype(newMask, newTypes);
        
        // Cache the edge
        m_archetypeEdges[currentArchetype][componentType].add = newArchetype;
        
        return newArchetype;
    }
    
    // Find archetype when removing a component
    uint32_t GetArchetypeAfterRemove(uint32_t currentArchetype, ComponentTypeID componentType) {
        // Ensure edges vector is large enough for current archetype
        if (currentArchetype >= m_archetypeEdges.size()) {
            UpdateArchetypeEdges(currentArchetype);
        }
        
        auto it = m_archetypeEdges[currentArchetype].find(componentType);
        if (it != m_archetypeEdges[currentArchetype].end()) {
            return it->second.remove;
        }
        
        // Calculate new archetype
        Archetype* current = GetArchetype(currentArchetype);
        if (!current || !current->HasComponent(componentType)) {
            return currentArchetype; // Doesn't have component
        }
        
        ComponentMask newMask = current->GetMask();
        newMask.reset(componentType);
        
        std::vector<ComponentTypeID> newTypes;
        for (ComponentTypeID type : current->GetComponentTypes()) {
            if (type != componentType) {
                newTypes.push_back(type);
            }
        }
        
        uint32_t newArchetype = GetOrCreateArchetype(newMask, newTypes);
        
        // Cache the edge
        m_archetypeEdges[currentArchetype][componentType].remove = newArchetype;
        
        return newArchetype;
    }
    
    // Get all archetypes (for queries)
    const std::vector<std::unique_ptr<Archetype>>& GetAllArchetypes() const {
        return m_archetypes;
    }
    
    // Get archetypes matching a component mask
    std::vector<uint32_t> GetArchetypesMatching(const ComponentMask& requiredMask, const ComponentMask& excludedMask) const {
        std::vector<uint32_t> matching;
        
        for (size_t i = 0; i < m_archetypes.size(); ++i) {
            const ComponentMask& archetypeMask = m_archetypes[i]->GetMask();
            
            // Check if has all required components
            if ((archetypeMask & requiredMask) == requiredMask) {
                // Check if has no excluded components
                if ((archetypeMask & excludedMask).none()) {
                    matching.push_back(static_cast<uint32_t>(i));
                }
            }
        }
        
        return matching;
    }
    
private:
    std::vector<std::unique_ptr<Archetype>> m_archetypes;
    std::vector<std::unordered_map<ComponentTypeID, ArchetypeEdge>> m_archetypeEdges;
    
    void UpdateArchetypeEdges(uint32_t newArchetypeIndex) {
        // Ensure edges vector is large enough
        while (m_archetypeEdges.size() <= newArchetypeIndex) {
            m_archetypeEdges.emplace_back();
        }
    }
};

} // namespace BGE