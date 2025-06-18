#pragma once

#include <cstdint>
#include <functional>

namespace BGE {

// Generational entity ID to prevent use-after-free bugs
struct EntityID {
    static constexpr uint32_t INDEX_BITS = 20;
    static constexpr uint32_t GENERATION_BITS = 12;
    static constexpr uint32_t INDEX_MASK = (1 << INDEX_BITS) - 1;
    static constexpr uint32_t GENERATION_MASK = (1 << GENERATION_BITS) - 1;
    static constexpr uint32_t GENERATION_SHIFT = INDEX_BITS;
    static constexpr uint32_t INVALID_INDEX = INDEX_MASK;
    
    uint32_t id;
    
    constexpr EntityID() : id(INVALID_INDEX) {}
    constexpr explicit EntityID(uint32_t index, uint32_t generation) 
        : id((generation << GENERATION_SHIFT) | (index & INDEX_MASK)) {}
    
    // Implicit constructor from uint64_t for backward compatibility
    EntityID(uint64_t legacyId) : id(static_cast<uint32_t>(legacyId)) {}
    
    uint32_t GetIndex() const { return id & INDEX_MASK; }
    uint32_t GetGeneration() const { return (id >> GENERATION_SHIFT) & GENERATION_MASK; }
    
    bool IsValid() const { return GetIndex() != INVALID_INDEX; }
    
    bool operator==(const EntityID& other) const { return id == other.id; }
    bool operator!=(const EntityID& other) const { return id != other.id; }
    bool operator<(const EntityID& other) const { return id < other.id; }
    
    // For backward compatibility
    operator uint64_t() const { return id; }
    
    static constexpr EntityID Invalid() { return EntityID(); }
};

// Invalid entity constant
constexpr EntityID INVALID_ENTITY = EntityID();

} // namespace BGE

// Hash function for EntityID
namespace std {
    template<>
    struct hash<BGE::EntityID> {
        size_t operator()(const BGE::EntityID& id) const {
            return std::hash<uint32_t>{}(id.id);
        }
    };
}