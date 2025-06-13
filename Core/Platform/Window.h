#pragma once

#include <string>
#include <functional>
#include <memory>

namespace BGE {

struct WindowConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    std::string title = "BGE Window";
    bool resizable = true;
    bool fullscreen = false;
    bool vsync = true;
};

struct WindowEvent {
    enum Type {
        Closed,
        Resized,
        KeyPressed,
        KeyReleased,
        MousePressed,
        MouseReleased,
        MouseMoved,
        MouseWheel
    };
    
    Type type;
    
    struct {
        int key;
    } keyboard;
    
    struct {
        int button;
        float x, y;
        float deltaX, deltaY;
    } mouse;
    
    struct {
        uint32_t width, height;
    } size;
};

using EventCallback = std::function<void(const WindowEvent&)>;

class WindowImpl;

class Window {
public:
    Window();
    ~Window();
    
    bool Initialize(const WindowConfig& config);
    void Shutdown();
    
    void PollEvents();
    void SwapBuffers();
    
    bool ShouldClose() const;
    void SetShouldClose(bool shouldClose);
    
    void GetSize(int& width, int& height) const;
    void SetSize(int width, int height);
    void GetFramebufferSize(int& width, int& height) const;
    void SetTitle(const std::string& title);
    
    void SetEventCallback(EventCallback callback) { m_eventCallback = callback; }
    
    // Input system connection
    void SetInputManager(class InputManager* inputManager);
    
    // Platform-specific handle
    void* GetNativeHandle() const;

private:
    EventCallback m_eventCallback;
    std::unique_ptr<WindowImpl> m_impl;
};

} // namespace BGE