#include "MaterialPalette.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include "../../Simulation/Materials/Material.h" // Required for Material class definition
#include <algorithm> // For std::remove_if
#include <iostream> // For potential debug output

namespace BGE {

MaterialPalette::MaterialPalette() = default;

void MaterialPalette::Initialize(MaterialSystem* materialSystem) {
    m_materialSystem = materialSystem;
    if (!m_materialSystem) {
        // std::cerr << "MaterialPalette Error: MaterialSystem is null during Initialize." << std::endl; // Optional: error logging
        return;
    }

    Clear();

    // Add Eraser (MATERIAL_EMPTY) manually as it's a special tool
    // Hotkey '0' (ASCII 48)
    PaletteMaterial eraser;
    eraser.id = MATERIAL_EMPTY;
    eraser.name = "Eraser";
    eraser.description = "Removes material";
    eraser.hotkey = 48;
    eraser.color = 0xFF808080; // Default gray, or a specific color for eraser
    m_materials.push_back(eraser);

    // Load materials from MaterialSystem
    const auto& systemMaterials = m_materialSystem->GetAllMaterials();
    for (const auto& matPtr : systemMaterials) {
        if (matPtr) {
            PaletteMaterial palMat;
            palMat.id = matPtr->GetID();
            palMat.name = matPtr->GetName();
            // Material class does not have a description, using name.
            palMat.description = matPtr->GetName();
            // Material class does not store hotkey. Defaulting to 0 (no hotkey).
            // This means hotkeys from materials.json won't be reflected here
            // until Material class is updated to store hotkeys.
            palMat.hotkey = matPtr->GetHotKey(); // Now GetHotKey() exists and should provide the value.
            // palMat.hotkey = 0; // Defaulting to 0 (no hotkey) as Material class does not currently store hotkey.
            palMat.color = matPtr->GetColor();
            m_materials.push_back(palMat);
        }
    }

    // Sort materials by ID to ensure consistent order, optional
    std::sort(m_materials.begin() + 1, m_materials.end(), [](const PaletteMaterial& a, const PaletteMaterial& b) {
        return a.id < b.id;
    });

    // Select the first available material (after Eraser) or Eraser if no other materials
    if (m_materials.size() > 1) {
        SelectMaterial(1); // Select the first actual material
    } else {
        SelectMaterial(0); // Select Eraser
    }
}

// This AddMaterial is problematic as Palette should reflect MaterialSystem.
// Kept for now if direct manipulation is ever needed, but generally should not be used.
// Consider removing if palette is strictly derived from MaterialSystem.
void MaterialPalette::AddMaterial(MaterialID id, const std::string& name, const std::string& description, int hotkey) {
    // This function is mostly a remnant of the old system.
    // Its direct usage might lead to inconsistencies with MaterialSystem.
    // For now, it will add to the local list but without affecting MaterialSystem.
    PaletteMaterial material;
    material.id = id;
    material.name = name;
    material.description = description;
    material.hotkey = hotkey;
    material.color = 0xFF808080; // Default gray color
    
    if (m_materialSystem) {
        const Material* mat = m_materialSystem->GetMaterialPtr(id);
        if (mat) {
            material.color = mat->GetColor();
        }
    }
    m_materials.push_back(material);
}


void MaterialPalette::RemoveMaterial(MaterialID id) {
    // Similar to AddMaterial, direct removal can cause desync with MaterialSystem.
    // This should ideally be triggered by changes in MaterialSystem.
    auto it = std::remove_if(m_materials.begin(), m_materials.end(),
        [id](const PaletteMaterial& mat) { return mat.id == id; });
    if (it != m_materials.end()) {
        // If the removed material was selected, select Eraser
        if (m_materials[m_selectedIndex].id == id) {
            SelectMaterial(0); // Select Eraser
        }
        m_materials.erase(it, m_materials.end());
        // Adjust selectedIndex if necessary, though SelectMaterial(0) handles one case.
    }
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
            return; // Found and selected
        }
    }
    // If ID not found, perhaps select Eraser or do nothing
    // SelectMaterial(0);
}

void MaterialPalette::SelectMaterialByHotkey(int key) {
    // Hotkeys from JSON are not currently stored in Material objects.
    // This will only work for Eraser (hotkey '0') or if PaletteMaterial.hotkey is populated
    // through some other means (e.g. future update to Material class).
    for (size_t i = 0; i < m_materials.size(); ++i) {
        if (m_materials[i].hotkey == key) {
            SelectMaterial(i);
            return; // Found and selected
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

// CreateDefaultPalette is removed as materials are now loaded from MaterialSystem.
// void MaterialPalette::CreateDefaultPalette() { ... }


// UpdateMaterialColors might be redundant if colors are correctly set during Initialize
// and MaterialPalette is refreshed when MaterialSystem changes.
// If MaterialSystem can change material colors dynamically *after* palette initialization,
// then this or a similar refresh mechanism is needed.
void MaterialPalette::UpdateMaterialColors() {
    if (!m_materialSystem) return;
    
    for (auto& paletteMat : m_materials) {
        if (paletteMat.id == MATERIAL_EMPTY) continue; // Skip Eraser or handle its color if needed

        const Material* systemMat = m_materialSystem->GetMaterialPtr(paletteMat.id);
        if (systemMat) {
            paletteMat.color = systemMat->GetColor();
        } else {
            // Material no longer in system? Mark it or use a default 'missing' color.
            // paletteMat.color = 0xFFFF00FF; // e.g., bright pink for missing
        }
    }
}

} // namespace BGE