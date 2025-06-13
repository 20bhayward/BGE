#pragma once

#include "Keyboard.h"
#include "Mouse.h"

namespace BGE {

class InputManager {
public:
    InputManager();
    ~InputManager();
    
    bool Initialize();
    void Shutdown();
    void Update();
    
    // Keyboard input
    bool IsKeyPressed(int key) const;
    bool IsKeyJustPressed(int key) const;
    bool IsKeyJustReleased(int key) const;
    
    // Mouse input
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonJustPressed(int button) const;
    bool IsMouseButtonJustReleased(int button) const;
    
    void GetMousePosition(float& x, float& y) const;
    void GetMouseDelta(float& dx, float& dy) const;
    float GetMouseWheel() const;
    
    // Application callback connection
    void SetApplication(class Application* app);
    
    // Internal callbacks (called by window system)
    void OnKeyPressed(int key);
    void OnKeyReleased(int key);
    void OnMousePressed(int button);
    void OnMouseReleased(int button);
    void OnMouseMoved(float x, float y);
    void OnMouseWheel(float delta);

private:
    Keyboard m_keyboard;
    Mouse m_mouse;
    class Application* m_application = nullptr;
};

} // namespace BGE