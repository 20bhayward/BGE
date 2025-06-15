#include "AssetBrowserPanel.h"
#include "../Services.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace BGE {

AssetBrowserPanel::AssetBrowserPanel(const std::string& name, MaterialTools* materialTools)
    : Panel(name, PanelDockPosition::Left)
    , m_materialTools(materialTools) {
}

void AssetBrowserPanel::Initialize() {
    // Set window flags for asset browser
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
}

void AssetBrowserPanel::OnRender() {
    // Search bar
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##Search", m_searchBuffer, sizeof(m_searchBuffer))) {
        // Search changed
    }
    
    ImGui::Separator();
    
    // Tabs for different asset types
    if (ImGui::BeginTabBar("AssetTypes")) {
        if (ImGui::BeginTabItem("Materials")) {
            m_selectedTab = 0;
            RenderMaterialsTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Powers")) {
            m_selectedTab = 1;
            RenderPowersTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Textures")) {
            m_selectedTab = 2;
            RenderTexturesTab();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}

void AssetBrowserPanel::RenderMaterialsTab() {
    // Category filter
    const auto& categorizedPalette = m_materialTools->GetCategorizedPalette();
    auto categories = categorizedPalette.GetAvailableCategories();
    
    if (ImGui::BeginCombo("Category", categorizedPalette.GetCategoryName(m_filterCategory).c_str())) {
        for (auto category : categories) {
            if (category == MaterialCategory::Powers) continue; // Powers have their own tab
            
            bool isSelected = (m_filterCategory == category);
            if (ImGui::Selectable(categorizedPalette.GetCategoryName(category).c_str(), isSelected)) {
                m_filterCategory = category;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Thumbnail size slider
    ImGui::SliderFloat("Size", &m_thumbnailSize, 32.0f, 128.0f);
    
    ImGui::Separator();
    
    // Get materials to display
    std::vector<PaletteMaterial> materialsToShow;
    
    if (m_filterCategory == MaterialCategory::All) {
        // Show all materials from palette
        const auto& palette = m_materialTools->GetPalette();
        const auto& allMaterials = palette.GetMaterials();
        materialsToShow.insert(materialsToShow.end(), allMaterials.begin(), allMaterials.end());
    } else {
        // Show materials from specific category
        const auto& categoryMaterials = categorizedPalette.GetMaterialsInCategory(m_filterCategory);
        for (const auto& catMat : categoryMaterials) {
            PaletteMaterial palMat;
            palMat.id = catMat.id;
            palMat.name = catMat.name;
            palMat.description = catMat.description;
            palMat.hotkey = catMat.hotkey;
            palMat.color = catMat.color;
            materialsToShow.push_back(palMat);
        }
    }
    
    // Filter by search
    if (strlen(m_searchBuffer) > 0) {
        std::string searchLower = m_searchBuffer;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
        
        materialsToShow.erase(
            std::remove_if(materialsToShow.begin(), materialsToShow.end(),
                [&searchLower](const PaletteMaterial& mat) {
                    std::string nameLower = mat.name;
                    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                    return nameLower.find(searchLower) == std::string::npos;
                }),
            materialsToShow.end()
        );
    }
    
    // Render material grid
    RenderMaterialGrid(materialsToShow);
}

void AssetBrowserPanel::RenderMaterialGrid(const std::vector<PaletteMaterial>& materials) {
    float windowWidth = ImGui::GetContentRegionAvail().x;
    m_columns = std::max(1, static_cast<int>(windowWidth / (m_thumbnailSize + ImGui::GetStyle().ItemSpacing.x)));
    
    ImGui::Columns(m_columns, nullptr, false);
    
    for (size_t i = 0; i < materials.size(); ++i) {
        const PaletteMaterial& material = materials[i];
        
        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginGroup();
        
        // Material thumbnail button
        float r = ((material.color >> 0) & 0xFF) / 255.0f;
        float g = ((material.color >> 8) & 0xFF) / 255.0f;
        float b = ((material.color >> 16) & 0xFF) / 255.0f;
        float a = ((material.color >> 24) & 0xFF) / 255.0f;
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, a));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, a));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(r * 0.8f, g * 0.8f, b * 0.8f, a));
        
        if (ImGui::Button("##thumbnail", ImVec2(m_thumbnailSize, m_thumbnailSize))) {
            // Select material
            m_materialTools->GetBrush().SetMaterial(material.id);
            
            // Find and select in palette
            const auto& palette = m_materialTools->GetPalette();
            const auto& allMaterials = palette.GetMaterials();
            for (size_t j = 0; j < allMaterials.size(); ++j) {
                if (allMaterials[j].id == material.id) {
                    m_materialTools->GetPalette().SelectMaterial(j);
                    break;
                }
            }
        }
        
        ImGui::PopStyleColor(3);
        
        // Drag source for drag-and-drop
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MATERIAL_ID", &material.id, sizeof(MaterialID));
            ImGui::Text("%s", material.name.c_str());
            ImGui::EndDragDropSource();
        }
        
        // Material name
        ImGui::TextWrapped("%s", material.name.c_str());
        
        // Tooltip with description
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n\nHotkey: %c", 
                material.description.c_str(),
                material.hotkey != -1 ? static_cast<char>(material.hotkey) : '-');
        }
        
        ImGui::EndGroup();
        ImGui::PopID();
        
        ImGui::NextColumn();
    }
    
    ImGui::Columns(1);
}

void AssetBrowserPanel::RenderPowersTab() {
    const auto& categorizedPalette = m_materialTools->GetCategorizedPalette();
    const auto& powerTools = categorizedPalette.GetPowerTools();
    
    for (size_t i = 0; i < powerTools.size(); ++i) {
        const auto& power = powerTools[i];
        
        ImGui::PushID(static_cast<int>(i));
        
        // Power type icon
        const char* icon = "⚡";
        switch (power.type) {
            case PowerTool::PowerType::Bolt: icon = "⚡"; break;
            case PowerTool::PowerType::Explosion: icon = "💥"; break;
            case PowerTool::PowerType::Spray: icon = "🔥"; break;
            case PowerTool::PowerType::Brush: icon = "🖌"; break;
        }
        
        // Power button
        bool isSelected = (categorizedPalette.GetSelectedPower() == &power);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        }
        
        if (ImGui::Button((std::string(icon) + " " + power.name).c_str(), ImVec2(-1, 0))) {
            // Select this power
            m_materialTools->GetCategorizedPalette().SelectPower(i);
            if (power.materialId != MATERIAL_EMPTY) {
                m_materialTools->GetBrush().SetMaterial(power.materialId);
            }
        }
        
        if (isSelected) {
            ImGui::PopStyleColor();
        }
        
        // Show hotkey if available
        if (power.hotkey != -1) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%c)", static_cast<char>(power.hotkey));
        }
        
        // Tooltip with description
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", power.description.c_str());
        }
        
        ImGui::PopID();
    }
}

void AssetBrowserPanel::RenderTexturesTab() {
    ImGui::TextWrapped("Texture assets will be displayed here once the asset management system is fully integrated.");
    
    // TODO: Integrate with AssetManager to show loaded textures
    auto assetManager = Services::GetAssets();
    if (assetManager) {
        ImGui::Text("Asset Manager is available");
        // Future: Display texture thumbnails
    } else {
        ImGui::TextDisabled("Asset Manager not available");
    }
}

} // namespace BGE