#include "MaterialEditorUI.h"
#include "../Input/MaterialTools.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Simulation/Materials/MaterialSystem.h"

#include <imgui.h>
#include <iostream>

namespace BGE {

MaterialEditorUI::MaterialEditorUI() = default;

void MaterialEditorUI::Initialize(MaterialTools* tools, SimulationWorld* world) {
    m_materialTools = tools;
    m_world = world;
    
    std::cout << "MaterialEditorUI initialized" << std::endl;
}

void MaterialEditorUI::Render() {
    if (!m_visible || !m_materialTools || !m_world) {
        return;
    }
    
    RenderMainMenuBar();
    
    if (m_showMaterialPalette) {
        RenderMaterialPalette();
    }
    
    if (m_showSimulationControls) {
        RenderSimulationControls();
    }
    
    if (m_showBrushSettings) {
        RenderBrushSettings();
    }
    
    if (m_showStatusPanel) {
        RenderStatusPanel();
    }
    
    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
}

void MaterialEditorUI::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                m_world->Clear();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // TODO: Request engine shutdown
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Simulation")) {
            bool isPaused = m_world->IsPaused();
            if (ImGui::MenuItem("Play", "P", !isPaused)) {
                if (isPaused) m_world->Play();
            }
            if (ImGui::MenuItem("Pause", "P", isPaused)) {
                if (!isPaused) m_world->Pause();
            }
            if (ImGui::MenuItem("Step", "S")) {
                m_world->Step();
            }
            if (ImGui::MenuItem("Reset", "R")) {
                m_world->Reset();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Material Palette", nullptr, &m_showMaterialPalette);
            ImGui::MenuItem("Simulation Controls", nullptr, &m_showSimulationControls);
            ImGui::MenuItem("Brush Settings", nullptr, &m_showBrushSettings);
            ImGui::MenuItem("Status Panel", nullptr, &m_showStatusPanel);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemoWindow);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            ToolMode currentMode = m_materialTools->GetToolMode();
            if (ImGui::MenuItem("Paint", "B", currentMode == ToolMode::Paint)) {
                m_materialTools->SetToolMode(ToolMode::Paint);
            }
            if (ImGui::MenuItem("Erase", "E", currentMode == ToolMode::Erase)) {
                m_materialTools->SetToolMode(ToolMode::Erase);
            }
            if (ImGui::MenuItem("Sample", "I", currentMode == ToolMode::Sample)) {
                m_materialTools->SetToolMode(ToolMode::Sample);
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void MaterialEditorUI::RenderMaterialPalette() {
    ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(m_paletteWidth, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Material Palette", &m_showMaterialPalette)) {
        const MaterialPalette& palette = m_materialTools->GetPalette();
        const auto& materials = palette.GetMaterials();
        
        ImGui::Text("Select Material:");
        ImGui::Separator();
        
        for (size_t i = 0; i < materials.size(); ++i) {
            const PaletteMaterial& material = materials[i];
            bool isSelected = (palette.GetSelectedIndex() == i);
            
            // Create material button with color
            ImGui::PushID(static_cast<int>(i));
            
            // Color swatch
            UI::MaterialColor("##color", material.color);
            ImGui::SameLine();
            
            // Material button
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            }
            
            if (ImGui::Button(material.name.c_str(), ImVec2(-1, 0))) {
                m_materialTools->GetPalette().SelectMaterial(i);
                m_materialTools->GetBrush().SetMaterial(material.id);
            }
            
            if (isSelected) {
                ImGui::PopStyleColor();
            }
            
            // Show hotkey
            if (material.hotkey != -1) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%c)", static_cast<char>(material.hotkey));
            }
            
            // Tooltip with description
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", material.description.c_str());
            }
            
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void MaterialEditorUI::RenderSimulationControls() {
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - m_controlsHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(m_paletteWidth, m_controlsHeight), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Simulation Controls", &m_showSimulationControls)) {
        bool isPaused = m_world->IsPaused();
        
        // Play/Pause button
        if (isPaused) {
            if (UI::ColoredButton("▶ Play", 0.2f, 0.8f, 0.2f)) {
                m_world->Play();
            }
        } else {
            if (UI::ColoredButton("⏸ Pause", 0.8f, 0.6f, 0.2f)) {
                m_world->Pause();
            }
        }
        
        ImGui::SameLine();
        if (UI::ColoredButton("⏭ Step", 0.2f, 0.6f, 0.8f)) {
            m_world->Step();
        }
        
        ImGui::SameLine();
        if (UI::ColoredButton("⏹ Reset", 0.8f, 0.2f, 0.2f)) {
            m_world->Reset();
        }
        
        UI::Separator();
        
        // Simulation speed
        static float simSpeed = 1.0f;
        if (ImGui::SliderFloat("Speed", &simSpeed, 0.1f, 3.0f, "%.1f")) {
            m_world->SetSimulationSpeed(simSpeed);
        }
        
        // Clear world button
        if (UI::ColoredButton("Clear World", 0.6f, 0.2f, 0.2f)) {
            m_world->Clear();
        }
    }
    ImGui::End();
}

void MaterialEditorUI::RenderBrushSettings() {
    ImGui::SetNextWindowPos(ImVec2(m_paletteWidth + 10, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Brush Settings", &m_showBrushSettings)) {
        MaterialBrush& brush = m_materialTools->GetBrush();
        
        // Tool mode
        ToolMode currentMode = m_materialTools->GetToolMode();
        const char* modeNames[] = { "Paint", "Erase", "Sample" };
        int modeIndex = static_cast<int>(currentMode);
        if (ImGui::Combo("Tool Mode", &modeIndex, modeNames, 3)) {
            m_materialTools->SetToolMode(static_cast<ToolMode>(modeIndex));
        }
        
        UI::Separator();
        
        // Brush size
        int brushSize = brush.GetSize();
        if (UI::SliderWithReset("Size", &brushSize, 1, 20, 5)) {
            brush.SetSize(brushSize);
        }
        
        // Brush shape
        BrushShape shape = brush.GetShape();
        const char* shapeNames[] = { "Circle", "Square" };
        int shapeIndex = static_cast<int>(shape);
        if (ImGui::Combo("Shape", &shapeIndex, shapeNames, 2)) {
            brush.SetShape(static_cast<BrushShape>(shapeIndex));
        }
        
        // Temperature
        float temperature = brush.GetTemperature();
        if (ImGui::SliderFloat("Temperature", &temperature, 0.0f, 1000.0f, "%.1f°C")) {
            brush.SetTemperature(temperature);
        }
    }
    ImGui::End();
}

void MaterialEditorUI::RenderStatusPanel() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 150), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Status", &m_showStatusPanel)) {
        // Simulation info
        UI::StatusText("Status", m_world->IsPaused() ? "PAUSED" : "RUNNING");
        UI::StatusText("Frame", std::to_string(m_world->GetUpdateCount()).c_str());
        UI::StatusText("Active Particles", std::to_string(m_world->GetActiveCells()).c_str());
        UI::StatusText("Update Time", (std::to_string(m_world->GetLastUpdateTime() * 1000.0f) + "ms").c_str());
        
        UI::Separator();
        
        // World info
        UI::StatusText("World Size", (std::to_string(m_world->GetWidth()) + "x" + std::to_string(m_world->GetHeight())).c_str());
        
        UI::Separator();
        
        // Material info
        const MaterialPalette& palette = m_materialTools->GetPalette();
        const PaletteMaterial* selectedMat = palette.GetMaterial(palette.GetSelectedIndex());
        if (selectedMat) {
            UI::StatusText("Selected", selectedMat->name.c_str());
        }
        
        // Tool info
        const char* toolNames[] = { "Paint", "Erase", "Sample" };
        UI::StatusText("Tool", toolNames[static_cast<int>(m_materialTools->GetToolMode())]);
        UI::StatusText("Brush Size", std::to_string(m_materialTools->GetBrush().GetSize()).c_str());
    }
    ImGui::End();
}

} // namespace BGE