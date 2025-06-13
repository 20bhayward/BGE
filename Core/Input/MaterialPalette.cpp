#include "MaterialPalette.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include <algorithm>

namespace BGE {

MaterialPalette::MaterialPalette() = default;

void MaterialPalette::Initialize(MaterialSystem* materialSystem) {
    m_materialSystem = materialSystem;
    CreateDefaultPalette();
}

void MaterialPalette::AddMaterial(MaterialID id, const std::string& name, const std::string& description, int hotkey) {
    PaletteMaterial material;
    material.id = id;
    material.name = name;
    material.description = description;
    material.hotkey = hotkey;
    material.color = 0xFF808080; // Default gray color
    
    // Get actual color from material system
    if (m_materialSystem) {
        const Material* mat = m_materialSystem->GetMaterialPtr(id);
        if (mat) {
            material.color = mat->GetColor();
        }
    }
    
    m_materials.push_back(material);
}

void MaterialPalette::RemoveMaterial(MaterialID id) {
    auto it = std::remove_if(m_materials.begin(), m_materials.end(),
        [id](const PaletteMaterial& mat) { return mat.id == id; });
    m_materials.erase(it, m_materials.end());
}

void MaterialPalette::Clear() {
    m_materials.clear();
    m_selectedMaterial = MATERIAL_EMPTY;
    m_selectedIndex = 0;
}

void MaterialPalette::SelectMaterial(size_t index) {
    if (index < m_materials.size()) {
        m_selectedIndex = index;
        m_selectedMaterial = m_materials[index].id;
    }
}

void MaterialPalette::SelectMaterialByID(MaterialID id) {
    for (size_t i = 0; i < m_materials.size(); ++i) {
        if (m_materials[i].id == id) {
            SelectMaterial(i);
            break;
        }
    }
}

void MaterialPalette::SelectMaterialByHotkey(int key) {
    for (size_t i = 0; i < m_materials.size(); ++i) {
        if (m_materials[i].hotkey == key) {
            SelectMaterial(i);
            break;
        }
    }
}

const PaletteMaterial* MaterialPalette::GetMaterial(size_t index) const {
    if (index < m_materials.size()) {
        return &m_materials[index];
    }
    return nullptr;
}

const PaletteMaterial* MaterialPalette::GetMaterialByID(MaterialID id) const {
    for (const auto& material : m_materials) {
        if (material.id == id) {
            return &material;
        }
    }
    return nullptr;
}

void MaterialPalette::CreateDefaultPalette() {
    Clear();
    
    // Add eraser first
    AddMaterial(MATERIAL_EMPTY, "Eraser", "Remove materials", '0');
    
    // Note: These material IDs will be created by the application
    // using MaterialSystem::CreateMaterial() calls
    AddMaterial(1, "Sand", "Falls and piles naturally", '1');
    AddMaterial(2, "Water", "Flows and finds its level", '2');
    AddMaterial(3, "Fire", "Spreads and burns materials", '3');
    AddMaterial(4, "Wood", "Static structure that burns", '4');
    AddMaterial(5, "Stone", "Heavy foundation material", '5');
    AddMaterial(6, "Oil", "Flammable liquid, floats on water", '6');
    AddMaterial(7, "Steam", "Hot gas that rises", '7');
    AddMaterial(8, "NaturalGas", "Light gas, rises quickly", '8');
    
    // Add additional materials without hotkeys (cycle through with number keys)
    AddMaterial(9, "ThickGas", "Heavy gas, spreads horizontally");
    AddMaterial(10, "Smoke", "Disperses widely in all directions");
    AddMaterial(11, "PoisonGas", "Dangerous gas with special reactions");
    AddMaterial(12, "Ash", "Residue from combustion");
    
    // Select sand by default
    SelectMaterial(1);
}

void MaterialPalette::UpdateMaterialColors() {
    if (!m_materialSystem) return;
    
    for (auto& material : m_materials) {
        const Material* mat = m_materialSystem->GetMaterialPtr(material.id);
        if (mat) {
            material.color = mat->GetColor();
        }
    }
}

} // namespace BGE