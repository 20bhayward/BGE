#include "ApplicationUI.h"
#include "../Services.h"
#include "../Logger.h"

namespace BGE {

ApplicationUI::ApplicationUI() = default;

bool ApplicationUI::Initialize(MaterialTools* tools, SimulationWorld* world) {
    if (m_initialized) {
        BGE_LOG_WARNING("ApplicationUI", "Already initialized");
        return true;
    }
    
    m_materialTools = tools;
    m_world = world;
    
    if (!m_materialTools || !m_world) {
        BGE_LOG_ERROR("ApplicationUI", "Invalid parameters - MaterialTools or SimulationWorld is null");
        return false;
    }
    
    // Create and initialize the existing MaterialEditorUI
    m_materialEditorUI = std::make_unique<MaterialEditorUI>();
    m_materialEditorUI->Initialize(m_materialTools, m_world);
    
    m_initialized = true;
    BGE_LOG_INFO("ApplicationUI", "Application UI initialized successfully (delegating to MaterialEditorUI)");
    return true;
}

void ApplicationUI::Shutdown() {
    if (!m_initialized) return;
    
    if (m_materialEditorUI) {
        m_materialEditorUI->Shutdown();
        m_materialEditorUI.reset();
    }
    
    m_materialTools = nullptr;
    m_world = nullptr;
    m_initialized = false;
    
    BGE_LOG_INFO("ApplicationUI", "Application UI shutdown complete");
}

void ApplicationUI::Render() {
    if (!m_visible || !m_initialized) {
        return;
    }
    
    // Delegate to the existing MaterialEditorUI
    if (m_materialEditorUI) {
        m_materialEditorUI->Render();
    }
}

std::shared_ptr<Panel> ApplicationUI::GetPanel(const std::string& name) const {
    if (!m_materialEditorUI) return nullptr;
    
    // Map common panel names to MaterialEditorUI getters
    if (name == "Hierarchy") return m_materialEditorUI->GetHierarchyPanel();
    if (name == "Inspector") return m_materialEditorUI->GetInspectorPanel();
    if (name == "Game" || name == "GameView") return m_materialEditorUI->GetGameViewPanel();
    if (name == "Scene View") return m_materialEditorUI->GetSceneViewPanel();
    if (name == "Sculpting") return m_materialEditorUI->GetSculptingPanel();
    if (name == "Game Panel") return m_materialEditorUI->GetGamePanel();
    if (name == "Asset Browser") return m_materialEditorUI->GetAssetBrowserPanel();
    if (name == "Materials") return m_materialEditorUI->GetMaterialPalettePanel();
    
    return nullptr;
}

void ApplicationUI::ShowPanel(const std::string& name, bool /*show*/) {
    // For now, panels are managed by the docking system
    // TODO: Implement panel visibility control
    BGE_LOG_INFO("ApplicationUI", "Panel visibility control not yet implemented: " + name);
}

void ApplicationUI::TogglePanel(const std::string& name) {
    // For now, panels are managed by the docking system
    // TODO: Implement panel toggle
    BGE_LOG_INFO("ApplicationUI", "Panel toggle not yet implemented: " + name);
}

} // namespace BGE