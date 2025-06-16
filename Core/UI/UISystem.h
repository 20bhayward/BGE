#pragma once

#include <memory>
#include <functional>
#include <imgui.h>
#include "LayoutInfo.h"
#include "DockingSystem.h"
#include "PanelManager.h"

namespace BGE {

class Window;
class Application;

class UISystem {
public:
    UISystem();
    ~UISystem();
    
    bool Initialize(Window* window);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    // UI state
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    
    // Input handling (called by application)
    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;
    
    // Style and theming
    void SetDarkTheme();
    void SetLightTheme();
    void SetCustomTheme();
    
    // Docking and viewport management
    void BeginDockspace();
    void EndDockspace();
    bool IsDockingEnabled() const;
    
    // Layout management
    const LayoutInfo& GetLayoutInfo() const { return m_layoutInfo; }
    
    // Modern docking system
    DockingSystem& GetDockingSystem() { return m_dockingSystem; }
    PanelManager& GetPanelManager() { return m_panelManager; }
    
    // Panel management shortcuts
    template<typename T, typename... Args>
    std::shared_ptr<T> RegisterPanel(const std::string& name, Args&&... args) {
        auto panel = m_panelManager.RegisterPanel<T>(name, std::forward<Args>(args)...);
        if (panel) {
            m_dockingSystem.AddPanel(panel);
        }
        return panel;
    }

private:
    bool m_enabled = true;
    bool m_initialized = false;
    Window* m_window = nullptr;
    LayoutInfo m_layoutInfo;
    
    // Modern docking system
    DockingSystem m_dockingSystem;
    PanelManager m_panelManager;
    
    void SetupStyle();
};

// UI Helper functions for common widgets
namespace UI {
    bool ColoredButton(const char* label, float r, float g, float b, float a = 1.0f);
    void MaterialColor(const char* label, uint32_t color);
    bool SliderWithReset(const char* label, int* value, int min, int max, int defaultValue);
    void StatusText(const char* label, const char* text);
    void Separator();
    void Spacing();
}

} // namespace BGE