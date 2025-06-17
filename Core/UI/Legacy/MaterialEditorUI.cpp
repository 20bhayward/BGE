#include "MaterialEditorUI.h"
#include "../Framework/UISystem.h"
#include "../../Input/MaterialTools.h"
#include "../../../Simulation/SimulationWorld.h"
#include "../../../Simulation/Materials/MaterialSystem.h"
#include "../../Services.h"
#include "../../../Renderer/PostProcessor.h"
#include "../../../Renderer/Renderer.h"
#include "../../../Renderer/ParticleSystem.h"
#include "../../../Renderer/PixelCamera.h"
#include "../Panels/AssetBrowserPanel.h"
#include "../Panels/ProjectSettingsPanel.h"
#include "../Panels/GameViewportPanel.h"
#include "../Panels/InspectorPanel.h"
#include "../Panels/HierarchyPanel.h"
#include "../Panels/MaterialPalettePanel.h"
#include "../Panels/MaterialEditorPanel.h"
#include "../Panels/ConsolePanel.h"
#include "../Docking/DockingSystem.h"
#include "../Docking/DockNode.h"

#include <imgui.h>
#include <iostream>
#include <algorithm>

namespace BGE {

MaterialEditorUI::MaterialEditorUI() = default;

void MaterialEditorUI::Initialize(MaterialTools* tools, SimulationWorld* world) {
    m_materialTools = tools;
    m_world = world;
    
    // Get the UI system to register panels with custom docking support
    auto uiSystem = Services::GetUI();
    if (!uiSystem) {
        std::cerr << "Failed to get UISystem service" << std::endl;
        return;
    }
    
    auto& docking = uiSystem->GetDockingSystem();
    
    // Create panels and add them to specific dock areas
    m_hierarchyPanel = std::make_shared<HierarchyPanel>("Hierarchy", m_world);
    m_hierarchyPanel->Initialize();
    docking.AddPanel(m_hierarchyPanel, "left");
    
    m_assetBrowserPanel = std::make_shared<AssetBrowserPanel>("Asset Browser");
    m_assetBrowserPanel->Initialize();
    docking.AddPanel(m_assetBrowserPanel, "bottom");
    
    m_gameViewportPanel = std::make_shared<GameViewportPanel>("Game", m_world, m_materialTools);
    m_gameViewportPanel->Initialize();
    docking.AddPanel(m_gameViewportPanel, "game");
    
    m_inspectorPanel = std::make_shared<InspectorPanel>("Inspector");
    m_inspectorPanel->Initialize();
    docking.AddPanel(m_inspectorPanel, "inspector");
    
    // Asset inspection is now integrated into the main InspectorPanel
    
    m_materialPalettePanel = std::make_shared<MaterialPalettePanel>("Materials", m_materialTools);
    m_materialPalettePanel->Initialize();
    docking.AddPanel(m_materialPalettePanel, "bottom");
    
    // Add Material Editor panel as placeholder
    auto materialEditorPanel = std::make_shared<MaterialEditorPanel>("MaterialEditor", m_materialTools);
    materialEditorPanel->Initialize();
    docking.AddPanel(materialEditorPanel, "inspector");
    
    // Add Console panel as placeholder
    auto consolePanel = std::make_shared<ConsolePanel>("Console");
    consolePanel->Initialize();
    docking.AddPanel(consolePanel, "bottom");
    
    // Create Project Settings panel as standalone window (not docked)
    m_projectSettingsPanel = std::make_unique<ProjectSettingsPanel>("Project Settings");
    m_projectSettingsPanel->Initialize();
    
    // Register with Services so other panels can access it
    Services::SetProjectSettings(m_projectSettingsPanel.get());
    
    std::cout << "MaterialEditorUI initialized with custom docking and placeholder panels" << std::endl;
}

void MaterialEditorUI::Shutdown() {
    // Shutdown panels
    if (m_hierarchyPanel) m_hierarchyPanel->Shutdown();
    if (m_assetBrowserPanel) m_assetBrowserPanel->Shutdown();
    if (m_gameViewportPanel) m_gameViewportPanel->Shutdown();
    if (m_inspectorPanel) m_inspectorPanel->Shutdown();
    if (m_materialPalettePanel) m_materialPalettePanel->Shutdown();
}

void MaterialEditorUI::Render() {
    if (!m_visible || !m_materialTools || !m_world) {
        return;
    }
    
    RenderMainMenuBar();
    
    // Render the custom docking system (which handles all panel rendering)
    auto uiSystem = Services::GetUI();
    if (uiSystem) {
        uiSystem->BeginDockspace();
    }
    
    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
    
    // Render standalone windows
    if (m_projectSettingsPanel) {
        m_projectSettingsPanel->Render();
    }
}


void MaterialEditorUI::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("New Scene")) {
                m_world->Clear();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Settings", "Ctrl+,")) {
                if (m_projectSettingsPanel) {
                    m_projectSettingsPanel->Show();
                }
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
        
        auto uiSystem = Services::GetUI();
        if (uiSystem) {
            uiSystem->RenderLayoutMenu();
            uiSystem->RenderWindowsMenu();
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
            ImGui::Separator();
            bool inspectorEnabled = m_materialTools->IsInspectorEnabled();
            if (ImGui::MenuItem("Toggle Inspector", "Q", inspectorEnabled)) {
                m_materialTools->SetInspectorEnabled(!inspectorEnabled);
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void MaterialEditorUI::SetupDockspace() {
    // The docking system is handled by UISystem
    // No need for manual positioning anymore
}

void MaterialEditorUI::RenderPanels() {
    // Panels are now rendered by the UISystem's docking system
    // This method is no longer needed
}

void MaterialEditorUI::SetupDefaultLayout() {
    // For now, let the docking system handle default layout
    // Just add panels and let them float initially - users can dock them manually
    std::cout << "Panels registered with docking system - users can arrange them by dragging!" << std::endl;
}

} // namespace BGE