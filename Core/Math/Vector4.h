#pragma once

#include <cmath>

namespace BGE {

struct Vector4 {
    float x, y, z, w;
    
    Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4(float value) : x(value), y(value), z(value), w(value) {}
    
    // Basic operations
    Vector4 operator+(const Vector4& other) const {
        return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    
    Vector4 operator-(const Vector4& other) const {
        return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    
    Vector4 operator*(float scalar) const {
        return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
    }
    
    Vector4 operator/(float scalar) const {
        return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
    }
    
    // Utility functions
    float Length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }
    
    float LengthSquared() const {
        return x * x + y * y + z * z + w * w;
    }
    
    Vector4 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return Vector4(x / len, y / len, z / len, w / len);
        }
        return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    float Dot(const Vector4& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
};

} // namespace BGE