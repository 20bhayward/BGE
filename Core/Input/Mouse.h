#pragma once

#include <array>

namespace BGE {

class Mouse {
public:
    Mouse();
    
    void Update();
    
    bool IsButtonPressed(int button) const;
    bool IsButtonJustPressed(int button) const;
    bool IsButtonJustReleased(int button) const;
    
    void GetPosition(float& x, float& y) const;
    void GetDelta(float& dx, float& dy) const;
    float GetWheelDelta() const;
    
    void SetButtonPressed(int button, bool pressed);
    void SetPosition(float x, float y);
    void SetWheelDelta(float delta);

private:
    static constexpr int MAX_BUTTONS = 8;
    
    std::array<bool, MAX_BUTTONS> m_currentButtons;
    std::array<bool, MAX_BUTTONS> m_previousButtons;
    
    float m_currentX, m_currentY;
    float m_previousX, m_previousY;
    float m_deltaX, m_deltaY;
    float m_wheelDelta;
    
    bool m_firstUpdate;
};

// Mouse button codes
namespace MouseButtons {
    constexpr int Left = 0;
    constexpr int Right = 1;
    constexpr int Middle = 2;
    constexpr int Button4 = 3;
    constexpr int Button5 = 4;
    constexpr int Button6 = 5;
    constexpr int Button7 = 6;
    constexpr int Button8 = 7;
}

} // namespace BGE