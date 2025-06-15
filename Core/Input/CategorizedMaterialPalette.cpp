#include "CategorizedMaterialPalette.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include <iostream>

namespace BGE {

CategorizedMaterialPalette::CategorizedMaterialPalette() = default;

void CategorizedMaterialPalette::Initialize(MaterialSystem* materialSystem) {
    m_materialSystem = materialSystem;
    CreateDefaultPalette();
    CreateDefaultPowerTools();
    UpdateMaterialColors();
    std::cout << "CategorizedMaterialPalette initialized with " << GetMaterialCount() << " materials and " << m_powerTools.size() << " powers" << std::endl;
}

void CategorizedMaterialPalette::AddMaterial(MaterialID id, const std::string& name, const std::string& description, 
                                           MaterialCategory category, int hotkey) {
    CategorizedMaterial material;
    material.id = id;
    material.name = name;
    material.description = description;
    material.category = category;
    material.hotkey = hotkey;
    material.color = 0xFFFFFFFF; // Will be updated later
    material.isPower = (category == MaterialCategory::Powers);
    
    m_categorizedMaterials[category].push_back(material);
}

void CategorizedMaterialPalette::AddPowerTool(const PowerTool& power) {
    m_powerTools.push_back(power);
}

void CategorizedMaterialPalette::Clear() {
    m_categorizedMaterials.clear();
    m_powerTools.clear();
}

void CategorizedMaterialPalette::SelectMaterial(MaterialCategory category, size_t index) {
    const auto& materials = GetMaterialsInCategory(category);
    if (index < materials.size()) {
        m_selectedMaterial = materials[index].id;
        m_selectedCategory = category;
        m_selectedPower = nullptr;
    }
}

void CategorizedMaterialPalette::SelectMaterialByID(MaterialID id) {
    for (const auto& [category, materials] : m_categorizedMaterials) {
        for (size_t i = 0; i < materials.size(); ++i) {
            if (materials[i].id == id) {
                SelectMaterial(category, i);
                return;
            }
        }
    }
}

void CategorizedMaterialPalette::SelectMaterialByHotkey(int key) {
    // Check materials first
    for (const auto& [category, materials] : m_categorizedMaterials) {
        for (size_t i = 0; i < materials.size(); ++i) {
            if (materials[i].hotkey == key) {
                SelectMaterial(category, i);
                return;
            }
        }
    }
    
    // Check power tools
    for (size_t i = 0; i < m_powerTools.size(); ++i) {
        if (m_powerTools[i].hotkey == key) {
            SelectPower(i);
            return;
        }
    }
}

void CategorizedMaterialPalette::SelectPower(size_t index) {
    if (index < m_powerTools.size()) {
        m_selectedPower = &m_powerTools[index];
        m_selectedMaterial = m_powerTools[index].materialId;
        m_selectedCategory = MaterialCategory::Powers;
    }
}

const std::vector<CategorizedMaterial>& CategorizedMaterialPalette::GetMaterialsInCategory(MaterialCategory category) const {
    static const std::vector<CategorizedMaterial> empty;
    auto it = m_categorizedMaterials.find(category);
    return (it != m_categorizedMaterials.end()) ? it->second : empty;
}

size_t CategorizedMaterialPalette::GetMaterialCount() const {
    size_t count = 0;
    for (const auto& [category, materials] : m_categorizedMaterials) {
        count += materials.size();
    }
    return count;
}

const CategorizedMaterial* CategorizedMaterialPalette::GetMaterial(MaterialCategory category, size_t index) const {
    const auto& materials = GetMaterialsInCategory(category);
    return (index < materials.size()) ? &materials[index] : nullptr;
}

const CategorizedMaterial* CategorizedMaterialPalette::GetMaterialByID(MaterialID id) const {
    for (const auto& [category, materials] : m_categorizedMaterials) {
        for (const auto& material : materials) {
            if (material.id == id) {
                return &material;
            }
        }
    }
    return nullptr;
}

std::vector<MaterialCategory> CategorizedMaterialPalette::GetAvailableCategories() const {
    std::vector<MaterialCategory> categories;
    for (const auto& [category, materials] : m_categorizedMaterials) {
        if (!materials.empty()) {
            categories.push_back(category);
        }
    }
    return categories;
}

std::string CategorizedMaterialPalette::GetCategoryName(MaterialCategory category) const {
    switch (category) {
        case MaterialCategory::All: return "All";
        case MaterialCategory::Solids: return "Solids";
        case MaterialCategory::Liquids: return "Liquids";
        case MaterialCategory::Powders: return "Powders";
        case MaterialCategory::Gases: return "Gases";
        case MaterialCategory::Special: return "Special";
        case MaterialCategory::Powers: return "Powers";
        default: return "Unknown";
    }
}

void CategorizedMaterialPalette::CreateDefaultPalette() {
    if (!m_materialSystem) return;
    
    // Solids
    AddMaterial(15, "Stone", "Basic rock material", MaterialCategory::Solids, 'G');
    AddMaterial(39, "Rock", "Basic rock formation - harder than stone", MaterialCategory::Solids);
    AddMaterial(40, "DenseRock", "Extremely hard rock formation - nearly indestructible", MaterialCategory::Solids);
    AddMaterial(41, "CursedRock", "Dark rock corrupted by void energy - spreads corruption", MaterialCategory::Solids);
    AddMaterial(4, "Wood", "Static structure that burns", MaterialCategory::Solids, '4');
    AddMaterial(18, "Metal", "Generic metal material", MaterialCategory::Solids, 'M');
    AddMaterial(23, "Concrete", "Construction material", MaterialCategory::Solids);
    AddMaterial(24, "Brick", "Building material", MaterialCategory::Solids);
    AddMaterial(25, "Coal", "Combustible rock", MaterialCategory::Solids);
    
    // Liquids
    AddMaterial(2, "Water", "Pure water - freezes, boils, and reacts", MaterialCategory::Liquids, '2');
    AddMaterial(9, "Oil", "Viscous liquid that floats on water", MaterialCategory::Liquids, '9');
    AddMaterial(12, "Lava", "Molten rock that burns and solidifies", MaterialCategory::Liquids, 'V');
    AddMaterial(11, "LiquidNitrogen", "Extremely cold liquid - freezes everything", MaterialCategory::Liquids, 'L');
    AddMaterial(13, "Acid", "Corrosive liquid that dissolves materials", MaterialCategory::Liquids, 'A');
    
    // Powders
    AddMaterial(1, "Sand", "Falls and piles naturally", MaterialCategory::Powders, '1');
    AddMaterial(16, "Dirt", "Common soil and earth", MaterialCategory::Powders, 'D');
    AddMaterial(5, "Ash", "Product of burning", MaterialCategory::Powders, '5');
    AddMaterial(20, "Snow", "Frozen water crystals", MaterialCategory::Powders, 'S');
    AddMaterial(29, "Dust", "Fine particles", MaterialCategory::Powders);
    AddMaterial(30, "Clay", "Moldable earth material", MaterialCategory::Powders);
    AddMaterial(19, "Gunpowder", "Explosive powder", MaterialCategory::Powders, 'P');
    
    // Gases
    AddMaterial(6, "Steam", "Rises from hot water", MaterialCategory::Gases, '6');
    AddMaterial(7, "Smoke", "Rises from fire", MaterialCategory::Gases, '7');
    AddMaterial(10, "Nitrogen", "Inert gas that extinguishes fire", MaterialCategory::Gases, 'N');
    AddMaterial(14, "ToxicGas", "Poisonous gas from acid reactions", MaterialCategory::Gases, 'T');
    AddMaterial(21, "Oxygen", "Life-supporting gas that feeds fire", MaterialCategory::Gases, 'O');
    AddMaterial(22, "Hydrogen", "Extremely light and explosive gas", MaterialCategory::Gases, 'H');
    
    // Special
    AddMaterial(8, "Ice", "Frozen water - melts back to water", MaterialCategory::Special, '8');
    AddMaterial(38, "EnchantedIce", "Magical ice that spreads and freezes surrounding materials", MaterialCategory::Special, 'F');
    AddMaterial(17, "Glass", "Transparent solid from melted sand", MaterialCategory::Special);
    AddMaterial(26, "Diamond", "Hardest natural material", MaterialCategory::Special);
    AddMaterial(27, "Obsidian", "Volcanic glass", MaterialCategory::Special);
    AddMaterial(33, "Crystal", "Grows and spreads when touching water", MaterialCategory::Special, 'E');
    
    // Powers (Special materials that require different handling)
    AddMaterial(3, "Fire", "Spreads and consumes fuel - short lifespan, hot and bright", MaterialCategory::Powers, '3');
    AddMaterial(34, "FrostFire", "Cold blue fire that freezes instead of burns", MaterialCategory::Powers, 'U');
    AddMaterial(35, "VoidFire", "Dark purple fire that consumes everything", MaterialCategory::Powers, 'Y');
    AddMaterial(36, "EternalFire", "Golden fire that never dies and spreads rapidly", MaterialCategory::Powers, 'I');
    AddMaterial(37, "Lightning", "Electrical energy that creates branching lightning lines", MaterialCategory::Powers, 'R');
}

void CategorizedMaterialPalette::CreateDefaultPowerTools() {
    // Lightning Bolt - Creates directed lightning
    PowerTool lightning;
    lightning.name = "Lightning Bolt";
    lightning.description = "Creates a powerful lightning bolt that electrifies materials";
    lightning.materialId = m_materialSystem ? m_materialSystem->GetMaterialID("Lightning") : MATERIAL_EMPTY;
    lightning.type = PowerTool::PowerType::Bolt;
    lightning.hotkey = 'Q';
    lightning.color = 0xFFFFFFFF;
    lightning.intensity = 80;
    lightning.range = 15;
    lightning.speed = 2.0f;
    m_powerTools.push_back(lightning);
    
    // Explosion
    PowerTool explosion;
    explosion.name = "Explosion";
    explosion.description = "Creates a devastating explosion that destroys materials";
    explosion.materialId = MATERIAL_EMPTY; // Special case - creates explosion effect
    explosion.type = PowerTool::PowerType::Explosion;
    explosion.hotkey = 'X';
    explosion.color = 0xFFFF8000;
    explosion.intensity = 100;
    explosion.range = 25;
    explosion.speed = 1.5f;
    m_powerTools.push_back(explosion);
    
    // Fire Spray
    PowerTool fireSpray;
    fireSpray.name = "Fire Spray";
    fireSpray.description = "Sprays fire in a wide area";
    fireSpray.materialId = m_materialSystem ? m_materialSystem->GetMaterialID("Fire") : MATERIAL_EMPTY;
    fireSpray.type = PowerTool::PowerType::Spray;
    fireSpray.hotkey = 'C';
    fireSpray.color = 0xFFFF4000;
    fireSpray.intensity = 60;
    fireSpray.range = 12;
    fireSpray.speed = 1.0f;
    m_powerTools.push_back(fireSpray);
}

void CategorizedMaterialPalette::UpdateMaterialColors() {
    if (!m_materialSystem) return;
    
    for (auto& [category, materials] : m_categorizedMaterials) {
        for (auto& material : materials) {
            const Material* mat = m_materialSystem->GetMaterialPtr(material.id);
            if (mat) {
                material.color = mat->GetColor();
            }
        }
    }
}

} // namespace BGE