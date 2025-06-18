#pragma once

#include "Vector3.h"
#include "Vector4.h"
#include <cmath>
#include <algorithm>

namespace BGE {

// Forward declaration
class Quaternion;

struct Matrix4 {
    union {
        float m[16]; // Column-major order
        float m2[4][4]; // 2D access
    };
    
    Matrix4() {
        Identity();
    }
    
    Matrix4(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33) {
        m[0] = m00; m[4] = m01; m[8]  = m02; m[12] = m03;
        m[1] = m10; m[5] = m11; m[9]  = m12; m[13] = m13;
        m[2] = m20; m[6] = m21; m[10] = m22; m[14] = m23;
        m[3] = m30; m[7] = m31; m[11] = m32; m[15] = m33;
    }
    
    void Identity() {
        m[0] = 1.0f; m[4] = 0.0f; m[8]  = 0.0f; m[12] = 0.0f;
        m[1] = 0.0f; m[5] = 1.0f; m[9]  = 0.0f; m[13] = 0.0f;
        m[2] = 0.0f; m[6] = 0.0f; m[10] = 1.0f; m[14] = 0.0f;
        m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
    }
    
    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i + j * 4] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result.m[i + j * 4] += m[i + k * 4] * other.m[k + j * 4];
                }
            }
        }
        return result;
    }
    
    Vector4 operator*(const Vector4& vec) const {
        return Vector4(
            m[0] * vec.x + m[4] * vec.y + m[8]  * vec.z + m[12] * vec.w,
            m[1] * vec.x + m[5] * vec.y + m[9]  * vec.z + m[13] * vec.w,
            m[2] * vec.x + m[6] * vec.y + m[10] * vec.z + m[14] * vec.w,
            m[3] * vec.x + m[7] * vec.y + m[11] * vec.z + m[15] * vec.w
        );
    }
    
    // Static creation functions
    static Matrix4 CreateIdentity() {
        Matrix4 mat;
        mat.Identity();
        return mat;
    }
    
    static Matrix4 Translation(const Vector3& translation) {
        Matrix4 mat;
        mat.m[12] = translation.x;
        mat.m[13] = translation.y;
        mat.m[14] = translation.z;
        return mat;
    }
    
    static Matrix4 Scale(const Vector3& scale) {
        Matrix4 mat;
        mat.m[0] = scale.x;
        mat.m[5] = scale.y;
        mat.m[10] = scale.z;
        return mat;
    }
    
    static Matrix4 RotationX(float radians) {
        Matrix4 mat;
        float c = std::cos(radians);
        float s = std::sin(radians);
        mat.m[5] = c;  mat.m[9] = -s;
        mat.m[6] = s;  mat.m[10] = c;
        return mat;
    }
    
    static Matrix4 RotationY(float radians) {
        Matrix4 mat;
        float c = std::cos(radians);
        float s = std::sin(radians);
        mat.m[0] = c;  mat.m[8] = s;
        mat.m[2] = -s; mat.m[10] = c;
        return mat;
    }
    
    static Matrix4 RotationZ(float radians) {
        Matrix4 mat;
        float c = std::cos(radians);
        float s = std::sin(radians);
        mat.m[0] = c;  mat.m[4] = -s;
        mat.m[1] = s;  mat.m[5] = c;
        return mat;
    }
    
    static Matrix4 Rotation(const Quaternion& q);
    
    static Matrix4 TRS(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
        Matrix4 t = Translation(position);
        Matrix4 r = Rotation(rotation);
        Matrix4 s = Scale(scale);
        return t * r * s;
    }
    
    static Matrix4 Perspective(float fovy, float aspect, float nearPlane, float farPlane) {
        Matrix4 mat;
        float tanHalfFovy = std::tan(fovy * 0.5f);
        mat.m[0] = 1.0f / (aspect * tanHalfFovy);
        mat.m[5] = 1.0f / tanHalfFovy;
        mat.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        mat.m[11] = -1.0f;
        mat.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        mat.m[15] = 0.0f;
        return mat;
    }
    
    static Matrix4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
        Matrix4 mat;
        mat.m[0] = 2.0f / (right - left);
        mat.m[5] = 2.0f / (top - bottom);
        mat.m[10] = -2.0f / (farPlane - nearPlane);
        mat.m[12] = -(right + left) / (right - left);
        mat.m[13] = -(top + bottom) / (top - bottom);
        mat.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        return mat;
    }
    
    static Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Vector3 zAxis = (eye - target).Normalized();
        Vector3 xAxis = up.Cross(zAxis).Normalized();
        Vector3 yAxis = zAxis.Cross(xAxis);
        
        Matrix4 mat;
        mat.m[0] = xAxis.x; mat.m[4] = xAxis.y; mat.m[8] = xAxis.z;
        mat.m[1] = yAxis.x; mat.m[5] = yAxis.y; mat.m[9] = yAxis.z;
        mat.m[2] = zAxis.x; mat.m[6] = zAxis.y; mat.m[10] = zAxis.z;
        mat.m[12] = -xAxis.Dot(eye);
        mat.m[13] = -yAxis.Dot(eye);
        mat.m[14] = -zAxis.Dot(eye);
        return mat;
    }
    
    // Matrix inverse
    Matrix4 Inverse() const {
        Matrix4 inv;
        float det;
        
        inv.m[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + 
                   m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        
        inv.m[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - 
                   m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        
        inv.m[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + 
                   m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        
        inv.m[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - 
                    m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        
        inv.m[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - 
                   m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        
        inv.m[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + 
                   m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        
        inv.m[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - 
                   m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        
        inv.m[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + 
                    m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        
        inv.m[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + 
                   m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        
        inv.m[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - 
                   m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        
        inv.m[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + 
                    m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        
        inv.m[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - 
                    m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        
        inv.m[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - 
                   m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        
        inv.m[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + 
                   m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        
        inv.m[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - 
                    m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        
        inv.m[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + 
                    m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
        
        det = m[0] * inv.m[0] + m[1] * inv.m[4] + m[2] * inv.m[8] + m[3] * inv.m[12];
        
        if (det == 0) {
            return CreateIdentity();
        }
        
        det = 1.0f / det;
        
        for (int i = 0; i < 16; i++) {
            inv.m[i] = inv.m[i] * det;
        }
        
        return inv;
    }
    
    // Decompose matrix into translation, rotation, and scale
    void Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const;
    
    // Convert matrix to float array (column-major order)
    void ToFloatArray(float* outArray) const {
        for (int i = 0; i < 16; ++i) {
            outArray[i] = m[i];
        }
    }
    
    // Get translation from matrix
    Vector3 GetTranslation() const {
        return Vector3(m[12], m[13], m[14]);
    }
    
    // Get scale from matrix
    Vector3 GetScale() const {
        float scaleX = Vector3(m[0], m[1], m[2]).Length();
        float scaleY = Vector3(m[4], m[5], m[6]).Length();
        float scaleZ = Vector3(m[8], m[9], m[10]).Length();
        return Vector3(scaleX, scaleY, scaleZ);
    }
    
    // Set translation in matrix
    void SetTranslation(const Vector3& translation) {
        m[12] = translation.x;
        m[13] = translation.y;
        m[14] = translation.z;
    }
    
    // Transform a point (w = 1)
    Vector3 TransformPoint(const Vector3& point) const {
        Vector4 v(point.x, point.y, point.z, 1.0f);
        Vector4 result = (*this) * v;
        if (result.w != 0.0f) {
            return Vector3(result.x / result.w, result.y / result.w, result.z / result.w);
        }
        return Vector3(result.x, result.y, result.z);
    }
    
    // Transform a direction (w = 0)
    Vector3 TransformDirection(const Vector3& direction) const {
        Vector4 v(direction.x, direction.y, direction.z, 0.0f);
        Vector4 result = (*this) * v;
        return Vector3(result.x, result.y, result.z);
    }
    
    // Access operators
    float& operator()(int row, int col) {
        return m[row + col * 4];
    }
    
    const float& operator()(int row, int col) const {
        return m[row + col * 4];
    }
};

} // namespace BGE