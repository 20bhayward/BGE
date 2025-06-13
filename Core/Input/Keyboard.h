#pragma once

#include <array>

namespace BGE {

class Keyboard {
public:
    Keyboard();
    
    void Update();
    
    bool IsKeyPressed(int key) const;
    bool IsKeyJustPressed(int key) const;
    bool IsKeyJustReleased(int key) const;
    
    void SetKeyPressed(int key, bool pressed);

private:
    static constexpr int MAX_KEYS = 512;
    
    std::array<bool, MAX_KEYS> m_currentKeys;
    std::array<bool, MAX_KEYS> m_previousKeys;
};

// Common key codes (compatible with GLFW)
namespace Keys {
    constexpr int Space = 32;
    constexpr int Apostrophe = 39;
    constexpr int Comma = 44;
    constexpr int Minus = 45;
    constexpr int Period = 46;
    constexpr int Slash = 47;
    constexpr int Key0 = 48;
    constexpr int Key1 = 49;
    constexpr int Key2 = 50;
    constexpr int Key3 = 51;
    constexpr int Key4 = 52;
    constexpr int Key5 = 53;
    constexpr int Key6 = 54;
    constexpr int Key7 = 55;
    constexpr int Key8 = 56;
    constexpr int Key9 = 57;
    constexpr int Semicolon = 59;
    constexpr int Equal = 61;
    constexpr int A = 65;
    constexpr int B = 66;
    constexpr int C = 67;
    constexpr int D = 68;
    constexpr int E = 69;
    constexpr int F = 70;
    constexpr int G = 71;
    constexpr int H = 72;
    constexpr int I = 73;
    constexpr int J = 74;
    constexpr int K = 75;
    constexpr int L = 76;
    constexpr int M = 77;
    constexpr int N = 78;
    constexpr int O = 79;
    constexpr int P = 80;
    constexpr int Q = 81;
    constexpr int R = 82;
    constexpr int S = 83;
    constexpr int T = 84;
    constexpr int U = 85;
    constexpr int V = 86;
    constexpr int W = 87;
    constexpr int X = 88;
    constexpr int Y = 89;
    constexpr int Z = 90;
    constexpr int Escape = 256;
    constexpr int Enter = 257;
    constexpr int Tab = 258;
    constexpr int Backspace = 259;
    constexpr int Insert = 260;
    constexpr int Delete = 261;
    constexpr int Right = 262;
    constexpr int Left = 263;
    constexpr int Down = 264;
    constexpr int Up = 265;
}

} // namespace BGE