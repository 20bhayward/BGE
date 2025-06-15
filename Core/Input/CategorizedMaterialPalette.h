#pragma once

#include "../../Simulation/Materials/Material.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace BGE {

class MaterialSystem;

enum class MaterialCategory {
    All,         // Show all materials
    Solids,      // Rock, Metal, Wood, etc.
    Liquids,     // Water, Oil, Acid, etc.
    Powders,     // Sand, Dirt, Ash, etc.
    Gases,       // Steam, Smoke, Oxygen, etc.
    Special,     // Crystal, Glass, Diamond, etc.
    Powers       // Lightning, Fire types, Explosions, etc.
};

struct CategorizedMaterial {
    MaterialID id;
    std::string name;
    std::string description;
    MaterialCategory category;
    int hotkey; // Key code for quick selection
    uint32_t color; // For UI display
    bool isPower = false; // True for special powers/abilities
};

struct PowerTool {
    std::string name;
    std::string description;
    MaterialID materialId;
    enum class PowerType {
        Brush,      // Apply like a brush (fires)
        Bolt,       // Single bolt/line (lightning)
        Explosion,  // Area effect explosion
        Spray       // Spread effect
    } type;
    int hotkey = -1;
    uint32_t color;
    
    // Power-specific settings
    int intensity = 50;     // 1-100
    int range = 10;         // Area of effect
    float speed = 1.0f;     // Animation/spread speed
};

class CategorizedMaterialPalette {
public:
    CategorizedMaterialPalette();
    ~CategorizedMaterialPalette() = default;
    
    void Initialize(MaterialSystem* materialSystem);
    
    // Material management
    void AddMaterial(MaterialID id, const std::string& name, const std::string& description, 
                    MaterialCategory category, int hotkey = -1);
    void AddPowerTool(const PowerTool& power);
    void Clear();
    
    // Selection
    void SelectMaterial(MaterialCategory category, size_t index);
    void SelectMaterialByID(MaterialID id);
    void SelectMaterialByHotkey(int key);
    void SelectPower(size_t index);
    
    MaterialID GetSelectedMaterial() const { return m_selectedMaterial; }
    const PowerTool* GetSelectedPower() const { return m_selectedPower; }
    MaterialCategory GetSelectedCategory() const { return m_selectedCategory; }
    
    // Access
    const std::vector<CategorizedMaterial>& GetMaterialsInCategory(MaterialCategory category) const;
    const std::vector<PowerTool>& GetPowerTools() const { return m_powerTools; }
    size_t GetMaterialCount() const;
    
    const CategorizedMaterial* GetMaterial(MaterialCategory category, size_t index) const;
    const CategorizedMaterial* GetMaterialByID(MaterialID id) const;
    
    // Category management
    std::vector<MaterialCategory> GetAvailableCategories() const;
    std::string GetCategoryName(MaterialCategory category) const;
    
    // Default material palette
    void CreateDefaultPalette();
    
private:
    std::unordered_map<MaterialCategory, std::vector<CategorizedMaterial>> m_categorizedMaterials;
    std::vector<PowerTool> m_powerTools;
    MaterialSystem* m_materialSystem = nullptr;
    
    MaterialID m_selectedMaterial = MATERIAL_EMPTY;
    const PowerTool* m_selectedPower = nullptr;
    MaterialCategory m_selectedCategory = MaterialCategory::Solids;
    
    void UpdateMaterialColors();
    void CreateDefaultPowerTools();
};

} // namespace BGE