#pragma once

#include <memory>
#include <functional>

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

private:
    bool m_enabled = true;
    bool m_initialized = false;
    Window* m_window = nullptr;
    
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