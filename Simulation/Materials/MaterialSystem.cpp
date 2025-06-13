#include "MaterialSystem.h"
#include <stdexcept>
#include <algorithm>

namespace BGE {

MaterialSystem::MaterialSystem() {
    // Reserve space for common materials
    m_materials.reserve(256);
    
    // Create the empty material (ID 0)
    auto emptyMaterial = std::make_unique<Material>(MATERIAL_EMPTY, "Empty");
    RegisterMaterial(std::move(emptyMaterial));
}

MaterialSystem::~MaterialSystem() = default;

MaterialID MaterialSystem::CreateMaterial(const std::string& name) {
    // Check if material already exists
    if (HasMaterial(name)) {
        return GetMaterialID(name);
    }
    
    MaterialID id = m_nextId++;
    auto material = std::make_unique<Material>(id, name);
    RegisterMaterial(std::move(material));
    
    return id;
}

Material& MaterialSystem::GetMaterial(MaterialID id) {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        throw std::runtime_error("Material with ID " + std::to_string(id) + " not found");
    }
    return *m_materials[it->second];
}

const Material& MaterialSystem::GetMaterial(MaterialID id) const {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        throw std::runtime_error("Material with ID " + std::to_string(id) + " not found");
    }
    return *m_materials[it->second];
}

Material* MaterialSystem::GetMaterialPtr(MaterialID id) {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        return nullptr;
    }
    return m_materials[it->second].get();
}

const Material* MaterialSystem::GetMaterialPtr(MaterialID id) const {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        return nullptr;
    }
    return m_materials[it->second].get();
}

MaterialID MaterialSystem::GetMaterialID(const std::string& name) const {
    auto it = m_nameToId.find(name);
    if (it == m_nameToId.end()) {
        return MATERIAL_EMPTY;
    }
    return it->second;
}

bool MaterialSystem::HasMaterial(const std::string& name) const {
    return m_nameToId.find(name) != m_nameToId.end();
}

bool MaterialSystem::HasMaterial(MaterialID id) const {
    return m_idToIndex.find(id) != m_idToIndex.end();
}

void MaterialSystem::LoadMaterialDatabase(const std::string& filepath) {
    (void)filepath;
    // TODO: Implement material database loading from file
    // This would load materials from a JSON/XML file
}

void MaterialSystem::SaveMaterialDatabase(const std::string& filepath) const {
    (void)filepath;
    // TODO: Implement material database saving to file
    // This would save materials to a JSON/XML file
}

bool MaterialSystem::ProcessReaction(MaterialID material1, MaterialID material2, 
                                   float temperature, MaterialID& product1, MaterialID& product2) const {
    const Material* mat1 = GetMaterialPtr(material1);
    const Material* mat2 = GetMaterialPtr(material2);
    
    if (!mat1 || !mat2) {
        return false;
    }
    
    // Check reactions from material1
    for (const auto& reaction : mat1->GetReactions()) {
        if (reaction.reactant == material2) {
            if (reaction.requiresHeat && temperature < reaction.minTemperature) {
                continue;
            }
            
            // Check probability
            if (static_cast<float>(rand()) / RAND_MAX < reaction.probability) {
                product1 = reaction.product1;
                product2 = reaction.product2;
                return true;
            }
        }
    }
    
    // Check reactions from material2
    for (const auto& reaction : mat2->GetReactions()) {
        if (reaction.reactant == material1) {
            if (reaction.requiresHeat && temperature < reaction.minTemperature) {
                continue;
            }
            
            // Check probability
            if (static_cast<float>(rand()) / RAND_MAX < reaction.probability) {
                product1 = reaction.product1;
                product2 = reaction.product2;
                return true;
            }
        }
    }
    
    return false;
}

void MaterialSystem::RegisterMaterial(std::unique_ptr<Material> material) {
    MaterialID id = material->GetID();
    const std::string& name = material->GetName();
    
    size_t index = m_materials.size();
    m_materials.push_back(std::move(material));
    m_idToIndex[id] = index;
    m_nameToId[name] = id;
}

} // namespace BGE