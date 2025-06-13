#include "Keyboard.h"

namespace BGE {

Keyboard::Keyboard() {
    m_currentKeys.fill(false);
    m_previousKeys.fill(false);
}

void Keyboard::Update() {
    m_previousKeys = m_currentKeys;
}

bool Keyboard::IsKeyPressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return m_currentKeys[key];
}

bool Keyboard::IsKeyJustPressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return m_currentKeys[key] && !m_previousKeys[key];
}

bool Keyboard::IsKeyJustReleased(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return !m_currentKeys[key] && m_previousKeys[key];
}

void Keyboard::SetKeyPressed(int key, bool pressed) {
    if (key >= 0 && key < MAX_KEYS) {
        m_currentKeys[key] = pressed;
    }
}

} // namespace BGE