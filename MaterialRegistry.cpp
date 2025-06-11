#include "MaterialRegistry.h"
// Removed iostream and algorithm includes as they are no longer needed.

MaterialRegistry::MaterialRegistry() {
    m_materials.clear(); // Clear existing materials, if any.
    // Resize to hold all MaterialID types. The size should be dynamic if more materials are added.
    // For now, hardcoding based on the current number of MaterialIDs.
    // Consider using MaterialID::Count or similar if the enum gets larger.
    m_materials.resize(4);

    m_materials[static_cast<size_t>(MaterialID::Empty)] = {MaterialID::Empty, "Empty", COLOR_EMPTY, 0.0f};
    m_materials[static_cast<size_t>(MaterialID::Sand)] = {MaterialID::Sand, "Sand", COLOR_SAND, 1.5f};
    m_materials[static_cast<size_t>(MaterialID::Rock)] = {MaterialID::Rock, "Rock", COLOR_ROCK, 2.0f};
    m_materials[static_cast<size_t>(MaterialID::Water)] = {MaterialID::Water, "Water", COLOR_WATER, 1.0f};
}

MaterialRegistry::~MaterialRegistry() {
    // Destructor remains the same, no specific cleanup needed for m_materials unless it holds pointers.
}

const MaterialDefinition& MaterialRegistry::getMaterial(MaterialID id) const {
    size_t index = static_cast<size_t>(id);
    if (index < m_materials.size()) {
        return m_materials[index];
    }
    // Fallback for invalid ID. This case should ideally not be reached if MaterialIDs are used correctly.
    // Returning Rock as a default, but this could also be an assertion or throw an error.
    // It might be better to return a const reference to a static default MaterialDefinition
    // or handle the error more explicitly depending on game's error handling strategy.
    return m_materials[static_cast<size_t>(MaterialID::Rock)];
}
