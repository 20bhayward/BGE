#pragma once

#include <cmath>
#include <algorithm>

namespace BGE {

namespace Math {
    // Constants
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float EPSILON = 1e-6f;
    
    // Utility functions
    template<typename T>
    inline T Clamp(T value, T min, T max) {
        return std::max(min, std::min(max, value));
    }
    
    template<typename T>
    inline T Lerp(T a, T b, float t) {
        return a + (b - a) * t;
    }
    
    inline float ToRadians(float degrees) {
        return degrees * DEG_TO_RAD;
    }
    
    inline float ToDegrees(float radians) {
        return radians * RAD_TO_DEG;
    }
    
    inline bool NearlyEqual(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
    
    inline float Smoothstep(float edge0, float edge1, float x) {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
    // Random number generation helpers
    inline float Random01() {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
    
    inline float RandomRange(float min, float max) {
        return min + Random01() * (max - min);
    }
    
    inline int RandomInt(int min, int max) {
        return min + rand() % (max - min + 1);
    }
}

} // namespace BGE