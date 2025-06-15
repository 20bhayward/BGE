#include "MaterialEditorUI.h"
#include "UISystem.h"
#include "../Input/MaterialTools.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include "../Services.h"
#include "../../Renderer/PostProcessor.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/ParticleSystem.h"
#include "../../Renderer/PixelCamera.h"
#include "AssetBrowserPanel.h"
#include "SceneViewPanel.h"
#include "GameViewportPanel.h"
#include "InspectorPanel.h"
#include "HierarchyPanel.h"
#include "MaterialPalettePanel.h"
#include "DebugToolbarPanel.h"

#include <imgui.h>
#include <iostream>

namespace BGE {

MaterialEditorUI::MaterialEditorUI() = default;

void MaterialEditorUI::Initialize(MaterialTools* tools, SimulationWorld* world) {
    m_materialTools = tools;
    m_world = world;
    
    // Initialize Unity-style layout manager
    m_layoutManager.Initialize();
    
    // Create panel instances
    m_hierarchyPanel = std::make_shared<HierarchyPanel>("Hierarchy", m_world);
    m_assetBrowserPanel = std::make_shared<AssetBrowserPanel>("Asset Browser", m_materialTools);
    m_debugToolbarPanel = std::make_shared<DebugToolbarPanel>("Debug Toolbar", m_world, m_materialTools);
    m_gameViewportPanel = std::make_shared<GameViewportPanel>("Game", m_world, m_materialTools);
    m_inspectorPanel = std::make_shared<InspectorPanel>("Inspector", m_materialTools, m_world);
    m_materialPalettePanel = std::make_shared<MaterialPalettePanel>("Materials", m_materialTools);
    
    // Initialize panels
    m_hierarchyPanel->Initialize();
    m_assetBrowserPanel->Initialize();
    m_debugToolbarPanel->Initialize();
    m_gameViewportPanel->Initialize();
    m_inspectorPanel->Initialize();
    m_materialPalettePanel->Initialize();
    
    // Set up default Unity-style layout
    SetupDefaultLayout();
    
    std::cout << "MaterialEditorUI initialized with Unity-style layout system" << std::endl;
}

void MaterialEditorUI::Shutdown() {
    // Shutdown panels
    if (m_hierarchyPanel) m_hierarchyPanel->Shutdown();
    if (m_assetBrowserPanel) m_assetBrowserPanel->Shutdown();
    if (m_debugToolbarPanel) m_debugToolbarPanel->Shutdown();
    if (m_gameViewportPanel) m_gameViewportPanel->Shutdown();
    if (m_inspectorPanel) m_inspectorPanel->Shutdown();
    if (m_materialPalettePanel) m_materialPalettePanel->Shutdown();
}

void MaterialEditorUI::Render() {
    if (!m_visible || !m_materialTools || !m_world) {
        return;
    }
    
    RenderMainMenuBar();
    
    // Always render Unity-style layout (don't depend on docking system)
    static bool debugPrinted = false;
    if (!debugPrinted) {
        std::cout << "MaterialEditorUI::Render() - About to render layout manager" << std::endl;
        debugPrinted = true;
    }
    m_layoutManager.Render();
    
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
            // Panel visibility toggles
            if (ImGui::MenuItem("Hierarchy", nullptr, m_hierarchyPanel->IsVisible())) {
                m_hierarchyPanel->ToggleVisible();
            }
            if (ImGui::MenuItem("Asset Browser", nullptr, m_assetBrowserPanel->IsVisible())) {
                m_assetBrowserPanel->ToggleVisible();
            }
            if (ImGui::MenuItem("Game Viewport", nullptr, m_gameViewportPanel->IsVisible())) {
                m_gameViewportPanel->ToggleVisible();
            }
            if (ImGui::MenuItem("Inspector", nullptr, m_inspectorPanel->IsVisible())) {
                m_inspectorPanel->ToggleVisible();
            }
            if (ImGui::MenuItem("Debug Toolbar", nullptr, m_debugToolbarPanel->IsVisible())) {
                m_debugToolbarPanel->ToggleVisible();
            }
            if (ImGui::MenuItem("Materials", nullptr, m_materialPalettePanel->IsVisible())) {
                m_materialPalettePanel->ToggleVisible();
            }
            
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

void MaterialEditorUI::SetupDefaultLayout() {
    // Add panels to their default Unity-style areas
    m_layoutManager.AddPanelToArea(m_hierarchyPanel, "Left");
    m_layoutManager.AddPanelToArea(m_assetBrowserPanel, "Left");  // Tab with hierarchy
    m_layoutManager.AddPanelToArea(m_debugToolbarPanel, "TopToolbar");
    m_layoutManager.AddPanelToArea(m_gameViewportPanel, "Center");
    m_layoutManager.AddPanelToArea(m_inspectorPanel, "Right");
    m_layoutManager.AddPanelToArea(m_materialPalettePanel, "Bottom");
}

} // namespace BGE