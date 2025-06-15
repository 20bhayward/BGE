#pragma once

#include <string>
#include <imgui.h>

namespace BGE {

enum class PanelDockPosition {
    Left,
    Right,
    Top,
    Bottom,
    Center,
    Floating
};

class Panel {
public:
    Panel(const std::string& name, PanelDockPosition defaultPosition = PanelDockPosition::Floating)
        : m_name(name)
        , m_visible(true)
        , m_defaultPosition(defaultPosition) {}
    
    virtual ~Panel() = default;
    
    // Called once when panel is created
    virtual void Initialize() {}
    
    // Called once when panel is destroyed
    virtual void Shutdown() {}
    
    // Called every frame to render the panel
    void Render() {
        if (!m_visible) return;
        
        // Begin window - let ImGui handle docking
        if (ImGui::Begin(m_name.c_str(), &m_visible, m_windowFlags)) {
            OnRender();
        }
        ImGui::End();
    }
    
    // Override this to render panel content
    virtual void OnRender() = 0;
    
    // Panel properties
    const std::string& GetName() const { return m_name; }
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void ToggleVisible() { m_visible = !m_visible; }
    
    // Window flags
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }
    ImGuiWindowFlags GetWindowFlags() const { return m_windowFlags; }
    
protected:
    
    std::string m_name;
    bool m_visible;
    PanelDockPosition m_defaultPosition;
    ImGuiWindowFlags m_windowFlags = 0;
};

} // namespace BGE