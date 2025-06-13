#pragma once

#include "../../Simulation/Materials/Material.h"
#include <vector>
#include <string>

namespace BGE {

class MaterialSystem;

struct PaletteMaterial {
    MaterialID id;
    std::string name;
    std::string description;
    int hotkey; // Key code for quick selection
    uint32_t color; // For UI display
};

class MaterialPalette {
public:
    MaterialPalette();
    ~MaterialPalette() = default;
    
    void Initialize(MaterialSystem* materialSystem);
    
    // Palette management
    void AddMaterial(MaterialID id, const std::string& name, const std::string& description, int hotkey = -1);
    void RemoveMaterial(MaterialID id);
    void Clear();
    
    // Selection
    void SelectMaterial(size_t index);
    void SelectMaterialByID(MaterialID id);
    void SelectMaterialByHotkey(int key);
    
    MaterialID GetSelectedMaterial() const { return m_selectedMaterial; }
    size_t GetSelectedIndex() const { return m_selectedIndex; }
    
    // Access
    const std::vector<PaletteMaterial>& GetMaterials() const { return m_materials; }
    size_t GetMaterialCount() const { return m_materials.size(); }
    
    const PaletteMaterial* GetMaterial(size_t index) const;
    const PaletteMaterial* GetMaterialByID(MaterialID id) const;
    
    // Default material palette
    void CreateDefaultPalette();
    
private:
    std::vector<PaletteMaterial> m_materials;
    MaterialSystem* m_materialSystem = nullptr;
    MaterialID m_selectedMaterial = MATERIAL_EMPTY;
    size_t m_selectedIndex = 0;
    
    void UpdateMaterialColors();
};

} // namespace BGE