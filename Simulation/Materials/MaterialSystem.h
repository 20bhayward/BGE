#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Material.h"

namespace BGE {

class MaterialSystem {
public:
    MaterialSystem();
    ~MaterialSystem();
    
    // Material creation and management
    MaterialID CreateMaterial(const std::string& name);
    Material& GetMaterial(MaterialID id);
    const Material& GetMaterial(MaterialID id) const;
    Material* GetMaterialPtr(MaterialID id);
    const Material* GetMaterialPtr(MaterialID id) const;
    
    // Material lookup
    MaterialID GetMaterialID(const std::string& name) const;
    bool HasMaterial(const std::string& name) const;
    bool HasMaterial(MaterialID id) const;
    
    // Material database operations
    void LoadMaterialDatabase(const std::string& filepath);
    void SaveMaterialDatabase(const std::string& filepath) const;
    
    // Material iteration
    size_t GetMaterialCount() const { return m_materials.size(); }
    const std::vector<std::unique_ptr<Material>>& GetAllMaterials() const { return m_materials; }
    
    // Reaction processing
    bool ProcessReaction(MaterialID material1, MaterialID material2, 
                        float temperature, MaterialID& product1, MaterialID& product2) const;
    
    // Helper for builder pattern
    class MaterialBuilder {
    public:
        MaterialBuilder(Material& material) : m_material(material) {}
        
        MaterialBuilder& SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            m_material.SetColor(r, g, b, a);
            return *this;
        }
        
        MaterialBuilder& SetBehavior(MaterialBehavior behavior) {
            m_material.SetBehavior(behavior);
            return *this;
        }
        
        MaterialBuilder& SetDensity(float density) {
            m_material.SetDensity(density);
            return *this;
        }
        
        MaterialBuilder& SetEmission(float emission) {
            m_material.SetEmission(emission);
            return *this;
        }
        
        MaterialBuilder& SetReactivity(float reactivity) {
            m_material.SetReactivity(reactivity);
            return *this;
        }
        
        MaterialBuilder& SetAcidity(float acidity) {
            m_material.SetAcidity(acidity);
            return *this;
        }
        
        MaterialBuilder& SetVolatility(float volatility) {
            m_material.SetVolatility(volatility);
            return *this;
        }
        
        MaterialID GetID() const {
            return m_material.GetID();
        }
        
        operator MaterialID() const {
            return m_material.GetID();
        }

        MaterialBuilder& SetHotKey(int hotkey) {
            m_material.m_hotkey = hotkey; // Directly set the hotkey via friend access
            return *this;
        }
        
    private:
        Material& m_material;
    };
    
    MaterialBuilder CreateMaterialBuilder(const std::string& name) {
        MaterialID id = CreateMaterial(name);
        return MaterialBuilder(GetMaterial(id));
    }

private:
    std::vector<std::unique_ptr<Material>> m_materials;
    std::unordered_map<std::string, MaterialID> m_nameToId;
    std::unordered_map<MaterialID, size_t> m_idToIndex;
    MaterialID m_nextId = 1; // 0 is reserved for MATERIAL_EMPTY
    
    void RegisterMaterial(std::unique_ptr<Material> material);
};

} // namespace BGE