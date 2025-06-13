#pragma once

#include <string>
#include <vector>
#include <memory>

namespace BGE {

class MaterialSystem;

class MaterialDatabase {
public:
    MaterialDatabase();
    ~MaterialDatabase();
    
    // Database operations
    bool LoadFromFile(const std::string& filepath, MaterialSystem& materialSystem);
    bool SaveToFile(const std::string& filepath, const MaterialSystem& materialSystem);
    
    // Predefined material sets
    void LoadBasicMaterials(MaterialSystem& materialSystem);
    void LoadAdvancedMaterials(MaterialSystem& materialSystem);
    void LoadChemicalMaterials(MaterialSystem& materialSystem);
    
    // Validation
    bool ValidateDatabase(const MaterialSystem& materialSystem) const;
    
private:
    struct MaterialData {
        std::string name;
        uint32_t color;
        int behavior;
        int state;
        float density;
        float meltingPoint;
        float boilingPoint;
        float ignitionPoint;
        float emission;
        float reflectivity;
        float transparency;
        float refractiveIndex;
        std::vector<std::string> reactions;
    };
    
    bool ParseMaterialData(const std::string& jsonData, std::vector<MaterialData>& materials);
    std::string SerializeMaterialData(const std::vector<MaterialData>& materials);
    void CreateMaterialFromData(const MaterialData& data, MaterialSystem& materialSystem);
};

} // namespace BGE