#pragma once

#include <string>
#include <iostream>
#include <functional>
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
        , m_defaultPosition(defaultPosition)
        , m_size(ImVec2(0, 0))
        , m_minSize(ImVec2(100, 100))
        , m_maxSize(ImVec2(FLT_MAX, FLT_MAX))
        , m_autoResize(true) {}
    
    virtual ~Panel() = default;
    
    // Called once when panel is created
    virtual void Initialize() {}
    
    // Called once when panel is destroyed
    virtual void Shutdown() {}
    
    // Called every frame to render the panel
    void Render() {
        if (!m_visible) return;
        
        bool wasVisible = m_visible;
        
        // Begin window - let ImGui handle docking
        if (ImGui::Begin(m_name.c_str(), &m_visible, m_windowFlags)) {
            OnRender();
        }
        ImGui::End();
        
        // If visibility changed (user clicked X), notify callback if set
        if (wasVisible != m_visible && m_visibilityChangedCallback) {
            m_visibilityChangedCallback(m_name, m_visible);
        }
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
    
    // Size management
    void SetSize(const ImVec2& size) { m_size = size; m_autoResize = false; }
    void SetSize(float width, float height) { SetSize(ImVec2(width, height)); }
    const ImVec2& GetSize() const { return m_size; }
    
    void SetMinSize(const ImVec2& minSize) { m_minSize = minSize; }
    void SetMinSize(float minWidth, float minHeight) { SetMinSize(ImVec2(minWidth, minHeight)); }
    const ImVec2& GetMinSize() const { return m_minSize; }
    
    void SetMaxSize(const ImVec2& maxSize) { m_maxSize = maxSize; }
    void SetMaxSize(float maxWidth, float maxHeight) { SetMaxSize(ImVec2(maxWidth, maxHeight)); }
    const ImVec2& GetMaxSize() const { return m_maxSize; }
    
    void SetAutoResize(bool autoResize) { m_autoResize = autoResize; }
    bool IsAutoResize() const { return m_autoResize; }
    
    // Visibility change callback
    void SetVisibilityChangedCallback(std::function<void(const std::string&, bool)> callback) {
        m_visibilityChangedCallback = callback;
    }
    
protected:
    
    std::string m_name;
    bool m_visible;
    PanelDockPosition m_defaultPosition;
    ImGuiWindowFlags m_windowFlags = 0;
    
    ImVec2 m_size;
    ImVec2 m_minSize;
    ImVec2 m_maxSize;
    bool m_autoResize;
    
    // Callback for when visibility changes (user clicks X button)
    std::function<void(const std::string&, bool)> m_visibilityChangedCallback;
};

} // namespace BGE