#pragma once

#include "Vector3.h"
#include "Matrix4.h"
#include <cmath>

namespace BGE {

class Quaternion {
public:
    float x, y, z, w;
    
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    
    static Quaternion Identity() { return Quaternion(0, 0, 0, 1); }
    
    static Quaternion FromAxisAngle(const Vector3& axis, float angle) {
        float halfAngle = angle * 0.5f;
        float s = sinf(halfAngle);
        return Quaternion(
            axis.x * s,
            axis.y * s,
            axis.z * s,
            cosf(halfAngle)
        );
    }
    
    static Quaternion FromEuler(float pitch, float yaw, float roll) {
        float cy = cosf(yaw * 0.5f);
        float sy = sinf(yaw * 0.5f);
        float cp = cosf(pitch * 0.5f);
        float sp = sinf(pitch * 0.5f);
        float cr = cosf(roll * 0.5f);
        float sr = sinf(roll * 0.5f);
        
        return Quaternion(
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        );
    }
    
    static Quaternion FromRotationMatrix(const Matrix4& m);
    
    static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3(0, 1, 0)) {
        Vector3 f = forward.Normalized();
        Vector3 r = up.Cross(f).Normalized();
        Vector3 u = f.Cross(r);
        
        float w = sqrtf(1.0f + r.x + u.y + f.z) * 0.5f;
        float w4_recip = 1.0f / (4.0f * w);
        
        return Quaternion(
            (u.z - f.y) * w4_recip,
            (f.x - r.z) * w4_recip,
            (r.y - u.x) * w4_recip,
            w
        );
    }
    
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        
        Quaternion q = b;
        if (dot < 0.0f) {
            q = Quaternion(-b.x, -b.y, -b.z, -b.w);
            dot = -dot;
        }
        
        if (dot > 0.9995f) {
            return Quaternion(
                a.x + t * (q.x - a.x),
                a.y + t * (q.y - a.y),
                a.z + t * (q.z - a.z),
                a.w + t * (q.w - a.w)
            ).Normalized();
        }
        
        float theta = acosf(dot);
        float sinTheta = sinf(theta);
        float wa = sinf((1.0f - t) * theta) / sinTheta;
        float wb = sinf(t * theta) / sinTheta;
        
        return Quaternion(
            wa * a.x + wb * q.x,
            wa * a.y + wb * q.y,
            wa * a.z + wb * q.z,
            wa * a.w + wb * q.w
        );
    }
    
    Quaternion operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y + y * other.w + z * other.x - x * other.z,
            w * other.z + z * other.w + x * other.y - y * other.x,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }
    
    Vector3 operator*(const Vector3& v) const {
        Vector3 qv(x, y, z);
        Vector3 t = qv.Cross(v) * 2.0f;
        return v + t * w + qv.Cross(t);
    }
    
    Quaternion Conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }
    
    Quaternion Inverse() const {
        float norm = x * x + y * y + z * z + w * w;
        return Quaternion(-x / norm, -y / norm, -z / norm, w / norm);
    }
    
    float Magnitude() const {
        return sqrtf(x * x + y * y + z * z + w * w);
    }
    
    Quaternion Normalized() const {
        float mag = Magnitude();
        if (mag > 0.0f) {
            float invMag = 1.0f / mag;
            return Quaternion(x * invMag, y * invMag, z * invMag, w * invMag);
        }
        return *this;
    }
    
    Vector3 ToEuler() const {
        float sinr_cosp = 2 * (w * x + y * z);
        float cosr_cosp = 1 - 2 * (x * x + y * y);
        float roll = atan2f(sinr_cosp, cosr_cosp);
        
        float sinp = 2 * (w * y - z * x);
        float pitch;
        if (fabs(sinp) >= 1)
            pitch = copysignf(3.14159265f / 2, sinp);
        else
            pitch = asinf(sinp);
        
        float siny_cosp = 2 * (w * z + x * y);
        float cosy_cosp = 1 - 2 * (y * y + z * z);
        float yaw = atan2f(siny_cosp, cosy_cosp);
        
        return Vector3(pitch, yaw, roll);
    }
    
    Matrix4 ToMatrix() const {
        float x2 = x * x;
        float y2 = y * y;
        float z2 = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;
        
        Matrix4 m;
        m.m[0] = 1.0f - 2.0f * (y2 + z2);
        m.m[1] = 2.0f * (xy + wz);
        m.m[2] = 2.0f * (xz - wy);
        m.m[3] = 0.0f;
        
        m.m[4] = 2.0f * (xy - wz);
        m.m[5] = 1.0f - 2.0f * (x2 + z2);
        m.m[6] = 2.0f * (yz + wx);
        m.m[7] = 0.0f;
        
        m.m[8] = 2.0f * (xz + wy);
        m.m[9] = 2.0f * (yz - wx);
        m.m[10] = 1.0f - 2.0f * (x2 + y2);
        m.m[11] = 0.0f;
        
        m.m[12] = 0.0f;
        m.m[13] = 0.0f;
        m.m[14] = 0.0f;
        m.m[15] = 1.0f;
        
        return m;
    }
};

} // namespace BGE