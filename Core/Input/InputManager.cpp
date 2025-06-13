#include "InputManager.h"
#include "../Application.h"

namespace BGE {

InputManager::InputManager() = default;
InputManager::~InputManager() = default;

bool InputManager::Initialize() {
    return true;
}

void InputManager::Shutdown() {
    // Cleanup if needed
}

void InputManager::Update() {
    m_keyboard.Update();
    m_mouse.Update();
}

bool InputManager::IsKeyPressed(int key) const {
    return m_keyboard.IsKeyPressed(key);
}

bool InputManager::IsKeyJustPressed(int key) const {
    return m_keyboard.IsKeyJustPressed(key);
}

bool InputManager::IsKeyJustReleased(int key) const {
    return m_keyboard.IsKeyJustReleased(key);
}

bool InputManager::IsMouseButtonPressed(int button) const {
    return m_mouse.IsButtonPressed(button);
}

bool InputManager::IsMouseButtonJustPressed(int button) const {
    return m_mouse.IsButtonJustPressed(button);
}

bool InputManager::IsMouseButtonJustReleased(int button) const {
    return m_mouse.IsButtonJustReleased(button);
}

void InputManager::GetMousePosition(float& x, float& y) const {
    m_mouse.GetPosition(x, y);
}

void InputManager::GetMouseDelta(float& dx, float& dy) const {
    m_mouse.GetDelta(dx, dy);
}

float InputManager::GetMouseWheel() const {
    return m_mouse.GetWheelDelta();
}

void InputManager::SetApplication(Application* app) {
    m_application = app;
}

void InputManager::OnKeyPressed(int key) {
    m_keyboard.SetKeyPressed(key, true);
    
    // Forward to application
    if (m_application) {
        m_application->OnKeyPressed(key);
    }
}

void InputManager::OnKeyReleased(int key) {
    m_keyboard.SetKeyPressed(key, false);
    
    // Forward to application
    if (m_application) {
        m_application->OnKeyReleased(key);
    }
}

void InputManager::OnMousePressed(int button) {
    m_mouse.SetButtonPressed(button, true);
    
    // Forward to application with current mouse position
    if (m_application) {
        float x, y;
        m_mouse.GetPosition(x, y);
        m_application->OnMousePressed(button, x, y);
    }
}

void InputManager::OnMouseReleased(int button) {
    m_mouse.SetButtonPressed(button, false);
    
    // Forward to application with current mouse position
    if (m_application) {
        float x, y;
        m_mouse.GetPosition(x, y);
        m_application->OnMouseReleased(button, x, y);
    }
}

void InputManager::OnMouseMoved(float x, float y) {
    m_mouse.SetPosition(x, y);
    
    // Forward to application
    if (m_application) {
        m_application->OnMouseMoved(x, y);
    }
}

void InputManager::OnMouseWheel(float delta) {
    m_mouse.SetWheelDelta(delta);
    
    // Forward to application
    if (m_application) {
        m_application->OnMouseWheel(delta);
    }
}

} // namespace BGE