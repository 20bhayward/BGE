#include "MaterialPalettePanel.h"
#include "../Services.h"
#include <imgui.h>

namespace BGE {

MaterialPalettePanel::MaterialPalettePanel(const std::string& name, MaterialTools* tools)
    : Panel(name, PanelDockPosition::Bottom)
    , m_tools(tools) {
}

void MaterialPalettePanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
}

void MaterialPalettePanel::OnRender() {
    if (!m_tools) return;
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
    
    // Horizontal layout for bottom panel
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float paletteWidth = panelWidth * 0.6f;
    float infoWidth = panelWidth * 0.4f;
    
    // Material grid on the left
    ImGui::BeginChild("MaterialGrid", ImVec2(paletteWidth, 0), true);
    RenderMaterialGrid();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Material info and brush settings on the right
    ImGui::BeginChild("MaterialInfo", ImVec2(infoWidth, 0), true);
    RenderSelectedMaterialInfo();
    ImGui::Separator();
    RenderBrushSettings();
    ImGui::EndChild();
    
    ImGui::PopStyleVar();
}

void MaterialPalettePanel::RenderMaterialGrid() {
    const MaterialPalette& palette = m_tools->GetPalette();
    size_t selectedIndex = palette.GetSelectedIndex();
    
    ImGui::Text("Material Palette");
    ImGui::Separator();
    
    // Calculate how many materials fit per row
    float availableWidth = ImGui::GetContentRegionAvail().x;
    int materialsPerRow = static_cast<int>((availableWidth - 20) / (m_materialButtonSize + 6));
    if (materialsPerRow < 1) materialsPerRow = 1;
    
    // Render material buttons in grid
    for (int i = 0; i < palette.GetMaterialCount(); ++i) {
        const PaletteMaterial* material = palette.GetMaterial(i);
        if (!material) continue;
        
        if (i > 0 && i % materialsPerRow != 0) {
            ImGui::SameLine();
        }
        
        // Material button with color
        uint32_t color = material->color;
        float r = ((color >> 0) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = ((color >> 16) & 0xFF) / 255.0f;
        
        bool isSelected = (selectedIndex == i);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r * 1.2f, g * 1.2f, b * 1.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.3f, g * 1.3f, b * 1.3f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.1f, g * 1.1f, b * 1.1f, 1.0f));
        }
        
        std::string buttonId = "##Mat" + std::to_string(i);
        if (ImGui::Button(buttonId.c_str(), ImVec2(m_materialButtonSize, m_materialButtonSize))) {
            m_tools->GetPalette().SelectMaterial(i);
        }
        
        if (isSelected) {
            ImGui::PopStyleVar();
        }
        ImGui::PopStyleColor(2);
        
        // Tooltip with material info
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", material->name.c_str());
            ImGui::Text("ID: %d", material->id);
            ImGui::Text("Description: %s", material->description.c_str());
            ImGui::EndTooltip();
        }
        
        // Drag source for materials
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MATERIAL_ID", &material->id, sizeof(MaterialID));
            ImGui::Text("Dragging %s", material->name.c_str());
            ImGui::EndDragDropSource();
        }
    }
}

void MaterialPalettePanel::RenderSelectedMaterialInfo() {
    const MaterialPalette& palette = m_tools->GetPalette();
    const PaletteMaterial* selectedMat = palette.GetMaterial(palette.GetSelectedIndex());
    
    ImGui::Text("Selected Material");
    ImGui::Separator();
    
    if (selectedMat) {
        // Material preview color
        uint32_t color = selectedMat->color;
        float r = ((color >> 0) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = ((color >> 16) & 0xFF) / 255.0f;
        
        ImGui::ColorButton("MaterialColor", ImVec4(r, g, b, 1.0f), 
                          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                          ImVec2(30, 30));
        ImGui::SameLine();
        
        // Material info
        ImGui::BeginGroup();
        ImGui::Text("%s", selectedMat->name.c_str());
        ImGui::Text("ID: %d", selectedMat->id);
        ImGui::Text("Description: %s", selectedMat->description.c_str());
        if (selectedMat->hotkey >= 0) {
            ImGui::Text("Hotkey: %d", selectedMat->hotkey);
        }
        ImGui::EndGroup();
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No material selected");
    }
}

void MaterialPalettePanel::RenderBrushSettings() {
    MaterialBrush& brush = m_tools->GetBrush();
    
    ImGui::Text("Brush Settings");
    ImGui::Separator();
    
    // Tool mode
    ToolMode currentMode = m_tools->GetToolMode();
    const char* modeNames[] = { "Paint", "Erase", "Sample" };
    int modeIndex = static_cast<int>(currentMode);
    
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("Tool", &modeIndex, modeNames, 3)) {
        m_tools->SetToolMode(static_cast<ToolMode>(modeIndex));
    }
    
    // Brush size
    int brushSize = brush.GetSize();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("Size", &brushSize, 1, 50)) {
        brush.SetSize(brushSize);
    }
    
    // Brush shape
    BrushShape shape = brush.GetShape();
    const char* shapeNames[] = { "Circle", "Square", "Triangle", "Diamond", "Line", "Cross", "Star", "Plus" };
    int shapeIndex = static_cast<int>(shape);
    
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("Shape", &shapeIndex, shapeNames, 8)) {
        brush.SetShape(static_cast<BrushShape>(shapeIndex));
    }
}

} // namespace BGE