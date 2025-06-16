#pragma once

#include <cstdint>

namespace BGE {

class Application {
public:
    virtual ~Application() = default;
    
    // Core application lifecycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    
    // Main loop callbacks
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    
    // Rendering control
    virtual bool HandlesWorldRendering() const { return false; }
    
    // Event callbacks
    virtual void OnWindowResize(uint32_t width, uint32_t height) { (void)width; (void)height; }
    virtual void OnWindowClose() {}
    
    // Input callbacks
    virtual void OnKeyPressed(int key) { (void)key; }
    virtual void OnKeyReleased(int key) { (void)key; }
    virtual void OnMousePressed(int button, float x, float y) { (void)button; (void)x; (void)y; }
    virtual void OnMouseReleased(int button, float x, float y) { (void)button; (void)x; (void)y; }
    virtual void OnMouseMoved(float x, float y) { (void)x; (void)y; }
    virtual void OnMouseWheel(float delta) { (void)delta; }
    
protected:
    Application() = default;
};

} // namespace BGE