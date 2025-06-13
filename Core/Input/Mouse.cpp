#include "Mouse.h"

namespace BGE {

Mouse::Mouse() 
    : m_currentX(0.0f), m_currentY(0.0f)
    , m_previousX(0.0f), m_previousY(0.0f)
    , m_deltaX(0.0f), m_deltaY(0.0f)
    , m_wheelDelta(0.0f)
    , m_firstUpdate(true) {
    
    m_currentButtons.fill(false);
    m_previousButtons.fill(false);
}

void Mouse::Update() {
    m_previousButtons = m_currentButtons;
    
    if (m_firstUpdate) {
        m_previousX = m_currentX;
        m_previousY = m_currentY;
        m_firstUpdate = false;
    }
    
    m_deltaX = m_currentX - m_previousX;
    m_deltaY = m_currentY - m_previousY;
    
    m_previousX = m_currentX;
    m_previousY = m_currentY;
    
    m_wheelDelta = 0.0f; // Reset wheel delta each frame
}

bool Mouse::IsButtonPressed(int button) const {
    if (button < 0 || button >= MAX_BUTTONS) return false;
    return m_currentButtons[button];
}

bool Mouse::IsButtonJustPressed(int button) const {
    if (button < 0 || button >= MAX_BUTTONS) return false;
    return m_currentButtons[button] && !m_previousButtons[button];
}

bool Mouse::IsButtonJustReleased(int button) const {
    if (button < 0 || button >= MAX_BUTTONS) return false;
    return !m_currentButtons[button] && m_previousButtons[button];
}

void Mouse::GetPosition(float& x, float& y) const {
    x = m_currentX;
    y = m_currentY;
}

void Mouse::GetDelta(float& dx, float& dy) const {
    dx = m_deltaX;
    dy = m_deltaY;
}

float Mouse::GetWheelDelta() const {
    return m_wheelDelta;
}

void Mouse::SetButtonPressed(int button, bool pressed) {
    if (button >= 0 && button < MAX_BUTTONS) {
        m_currentButtons[button] = pressed;
    }
}

void Mouse::SetPosition(float x, float y) {
    m_currentX = x;
    m_currentY = y;
}

void Mouse::SetWheelDelta(float delta) {
    m_wheelDelta = delta;
}

} // namespace BGE