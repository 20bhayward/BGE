#pragma once

#include <cmath>

namespace BGE {

struct Vector2 {
    float x, y;
    
    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}
    
    // Basic operations
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    
    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }
    
    Vector2 operator-() const {
        return Vector2(-x, -y);
    }
    
    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    
    Vector2 operator/(float scalar) const {
        return Vector2(x / scalar, y / scalar);
    }
    
    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    Vector2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    
    // Utility functions
    float Length() const {
        return std::sqrt(x * x + y * y);
    }
    
    float LengthSquared() const {
        return x * x + y * y;
    }
    
    Vector2 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return Vector2(x / len, y / len);
        }
        return Vector2(0.0f, 0.0f);
    }
    
    void Normalize() {
        float len = Length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
        }
    }
    
    float Dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }
    
    // Static utility functions
    static float Distance(const Vector2& a, const Vector2& b) {
        return (a - b).Length();
    }
    
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
        return a + (b - a) * t;
    }
};

} // namespace BGE