#include "MaterialEditorPanel.h"
#include "../../Input/MaterialTools.h"
#include <imgui.h>

namespace BGE {

MaterialEditorPanel::MaterialEditorPanel(const std::string& name, MaterialTools* tools)
    : Panel(name)
    , m_materialTools(tools) {
}

void MaterialEditorPanel::Initialize() {
    SetMinSize(250, 200);
}

void MaterialEditorPanel::OnRender() {
    if (!m_materialTools) {
        ImGui::Text("Material tools not available");
        return;
    }
    
    // Material selection
    ImGui::Text("Material Selection");
    ImGui::Separator();
    
    const char* materials[] = { "Stone", "Sand", "Water", "Lava", "Wood", "Metal", "Glass", "Concrete" };
    if (ImGui::BeginCombo("Material", materials[m_selectedMaterial])) {
        for (int i = 0; i < IM_ARRAYSIZE(materials); i++) {
            bool isSelected = (m_selectedMaterial == i);
            if (ImGui::Selectable(materials[i], isSelected)) {
                m_selectedMaterial = i;
                // m_materialTools->SetSelectedMaterial(i);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::Spacing();
    
    // Tool settings
    ImGui::Text("Brush Settings");
    ImGui::Separator();
    
    ImGui::SliderFloat("Size", &m_brushSize, 1.0f, 50.0f);
    ImGui::SliderFloat("Strength", &m_brushStrength, 0.1f, 1.0f);
    
    ImGui::Spacing();
    
    // Tool modes
    ImGui::Text("Tool Mode");
    ImGui::Separator();
    
    ToolMode currentMode = m_materialTools->GetToolMode();
    
    if (ImGui::RadioButton("Paint", currentMode == ToolMode::Paint)) {
        m_materialTools->SetToolMode(ToolMode::Paint);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Erase", currentMode == ToolMode::Erase)) {
        m_materialTools->SetToolMode(ToolMode::Erase);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Sample", currentMode == ToolMode::Sample)) {
        m_materialTools->SetToolMode(ToolMode::Sample);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Advanced settings
    if (ImGui::CollapsingHeader("Advanced Settings")) {
        bool inspectorEnabled = m_materialTools->IsInspectorEnabled();
        if (ImGui::Checkbox("Enable Inspector", &inspectorEnabled)) {
            m_materialTools->SetInspectorEnabled(inspectorEnabled);
        }
        
        ImGui::Text("Performance");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }
}

} // namespace BGE