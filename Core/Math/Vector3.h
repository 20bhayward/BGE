#pragma once

#include <cmath>

namespace BGE {

struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3(float value) : x(value), y(value), z(value) {}
    
    // Basic operations
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    Vector3 operator/(float scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }
    
    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    
    Vector3& operator-=(const Vector3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    
    Vector3& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    
    // Utility functions
    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    float LengthSquared() const {
        return x * x + y * y + z * z;
    }
    
    Vector3 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return Vector3(x / len, y / len, z / len);
        }
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    
    void Normalize() {
        float len = Length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
            z /= len;
        }
    }
    
    float Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    // Static utility functions
    static float Distance(const Vector3& a, const Vector3& b) {
        return (a - b).Length();
    }
    
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }
};

} // namespace BGE