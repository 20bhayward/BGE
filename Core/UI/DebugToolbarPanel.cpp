#include "DebugToolbarPanel.h"
#include "../Services.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/PixelCamera.h"
#include <imgui.h>
#include <string>

namespace BGE {

DebugToolbarPanel::DebugToolbarPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools)
    : Panel(name, PanelDockPosition::Top)
    , m_world(world)
    , m_tools(tools) {
}

void DebugToolbarPanel::Initialize() {
    // Make toolbar always visible, no title bar, no resize
    SetWindowFlags(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoScrollbar);
}

void DebugToolbarPanel::OnRender() {
    if (!m_world || !m_tools) return;
    
    // Horizontal layout with padding
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
    
    RenderSimulationControls();
    
    // Separator
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    RenderSimulationSettings();
    
    // Separator
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    RenderDebugControls();
    
    // Right-aligned performance info - use a simpler approach
    ImGui::SameLine();
    
    // Calculate available space and move cursor if there's room
    float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > 250) { // Need at least 250px for performance text
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - 240);
    }
    
    RenderPerformanceInfo();
    
    ImGui::PopStyleVar(2);
}

void DebugToolbarPanel::RenderSimulationControls() {
    // Simulation play/pause controls
    bool isPaused = m_world->IsPaused();
    
    if (isPaused) {
        if (ImGui::Button("▶ Play")) {
            m_world->Play();
        }
    } else {
        if (ImGui::Button("⏸ Pause")) {
            m_world->Pause();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏭ Step")) {
        m_world->Step();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏹ Reset")) {
        m_world->Reset();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("🗑 Clear")) {
        m_world->Clear();
    }
    
    // Simulation speed control
    ImGui::SameLine();
    ImGui::Text("Speed:");
    ImGui::SameLine();
    
    static float simSpeed = 1.0f;
    ImGui::SetNextItemWidth(80);
    if (ImGui::SliderFloat("##Speed", &simSpeed, 0.1f, 3.0f, "%.1fx")) {
        m_world->SetSimulationSpeed(simSpeed);
    }
}

void DebugToolbarPanel::RenderSimulationSettings() {
    // World size settings
    if (ImGui::Button("World")) {
        m_showWorldSizeDialog = !m_showWorldSizeDialog;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("World Settings");
    
    // Render world size dialog
    if (m_showWorldSizeDialog) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::Begin("World Settings", &m_showWorldSizeDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Current World Size: %dx%d", m_world->GetWidth(), m_world->GetHeight());
            ImGui::Separator();
            
            static int newWidth = 512;
            static int newHeight = 512;
            
            ImGui::SliderInt("Width", &newWidth, 256, 2048);
            ImGui::SliderInt("Height", &newHeight, 256, 2048);
            
            ImGui::Separator();
            
            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                // TODO: Implement world resize
                ImGui::OpenPopup("Resize Warning");
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                m_showWorldSizeDialog = false;
            }
            
            // Warning popup
            if (ImGui::BeginPopupModal("Resize Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Resizing the world will clear all current content!");
                ImGui::Text("This action cannot be undone.");
                ImGui::Separator();
                
                if (ImGui::Button("Confirm Resize", ImVec2(120, 0))) {
                    // Clear world and resize would go here
                    m_world->Clear();
                    ImGui::CloseCurrentPopup();
                    m_showWorldSizeDialog = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
    
    // Camera controls info
    ImGui::SameLine();
    ImGui::Text("Camera:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset")) {
        auto renderer = Services::GetRenderer();
        if (renderer && renderer->GetPixelCamera()) {
            renderer->GetPixelCamera()->SetPosition(Vector2{0, 0});
            renderer->GetPixelCamera()->SetZoom(1);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset camera position and zoom");
}

void DebugToolbarPanel::RenderDebugControls() {
    // Debug visualization toggles
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Stats", &m_showStats);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_showDebugInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Wire", &m_showWireframe);
    
    // Apply debug settings to game viewport
    // Note: These would typically be communicated to the GameViewportPanel
    // For now, we'll store them here and they can be accessed via getters
}

void DebugToolbarPanel::RenderPerformanceInfo() {
    // Compact performance display
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::SameLine();
    ImGui::Text("| Frame: %lld", static_cast<long long>(m_world->GetUpdateCount()));
    ImGui::SameLine();
    ImGui::Text("| Cells: %d", m_world->GetActiveCells());
}

} // namespace BGE