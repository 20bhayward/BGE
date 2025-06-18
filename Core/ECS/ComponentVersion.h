#pragma once

#include <atomic>
#include <cstdint>

namespace BGE {

// Global version counter for change tracking
class GlobalVersionCounter {
public:
    static uint64_t NextVersion() {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
};

// Version tracking for individual components
struct ComponentVersion {
    uint64_t version = 0;
    
    void Update() {
        version = GlobalVersionCounter::NextVersion();
    }
    
    bool IsNewerThan(uint64_t otherVersion) const {
        return version > otherVersion;
    }
    
    bool IsOlderThan(uint64_t otherVersion) const {
        return version < otherVersion;
    }
};

// Version tracking for archetypes
struct ArchetypeVersion {
    uint64_t structuralVersion = 0;  // Changes when entities are added/removed
    uint64_t componentVersion = 0;    // Changes when any component is modified
    
    void UpdateStructural() {
        structuralVersion = GlobalVersionCounter::NextVersion();
        componentVersion = structuralVersion; // Structural changes also update component version
    }
    
    void UpdateComponent() {
        componentVersion = GlobalVersionCounter::NextVersion();
    }
};

// Change tracking for entities
struct EntityChangeInfo {
    uint64_t lastModifiedVersion = 0;
    uint64_t lastStructuralChange = 0;
    bool wasDestroyed = false;
    
    void RecordModification() {
        lastModifiedVersion = GlobalVersionCounter::NextVersion();
    }
    
    void RecordStructuralChange() {
        lastStructuralChange = GlobalVersionCounter::NextVersion();
        lastModifiedVersion = lastStructuralChange;
    }
    
    void RecordDestruction() {
        wasDestroyed = true;
        RecordStructuralChange();
    }
};

} // namespace BGE