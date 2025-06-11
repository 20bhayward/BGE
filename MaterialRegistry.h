#pragma once

#include <string>
#include <vector> // Or some other suitable container

// Placeholder for Material type
struct Material {
    std::string name;
    // Add other material properties here (e.g., color, density, etc.)
};

class MaterialRegistry {
public:
    MaterialRegistry();
    ~MaterialRegistry();

    void registerMaterial(const Material& material);
    const Material* getMaterial(const std::string& name) const;
    // Add other material management methods here

private:
    std::vector<Material> m_materials;
};
