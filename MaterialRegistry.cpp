#include "MaterialRegistry.h"
#include <iostream> // For debug messages
#include <algorithm> // For std::find_if

MaterialRegistry::MaterialRegistry() {
    std::cout << "MaterialRegistry: Constructor called." << std::endl;
    // Initialize default materials or leave empty.
}

MaterialRegistry::~MaterialRegistry() {
    std::cout << "MaterialRegistry: Destructor called." << std::endl;
    // Clean up any resources if necessary.
}

void MaterialRegistry::registerMaterial(const Material& material) {
    // Check if material with the same name already exists to prevent duplicates
    auto it = std::find_if(m_materials.begin(), m_materials.end(),
                           [&](const Material& m) { return m.name == material.name; });

    if (it == m_materials.end()) {
        m_materials.push_back(material);
        std::cout << "MaterialRegistry: Registered material \"" << material.name << "\"." << std::endl;
    } else {
        std::cout << "MaterialRegistry: Material with name \"" << material.name << "\" already exists. Not registering." << std::endl;
    }
}

const Material* MaterialRegistry::getMaterial(const std::string& name) const {
    auto it = std::find_if(m_materials.begin(), m_materials.end(),
                           [&](const Material& m) { return m.name == name; });

    if (it != m_materials.end()) {
        return &(*it);
    } else {
        std::cout << "MaterialRegistry: Material with name \"" << name << "\" not found." << std::endl;
        return nullptr;
    }
}
