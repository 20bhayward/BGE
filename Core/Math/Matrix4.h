#pragma once

#include "Vector3.h"
#include "Vector4.h"
#include <cmath>

namespace BGE {

struct Matrix4 {
    float m[16]; // Column-major order
    
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
    
    static Matrix4 RotationZ(float radians) {
        Matrix4 mat;
        float c = std::cos(radians);
        float s = std::sin(radians);
        mat.m[0] = c;  mat.m[4] = -s;
        mat.m[1] = s;  mat.m[5] = c;
        return mat;
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
};

} // namespace BGE