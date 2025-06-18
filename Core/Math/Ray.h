#pragma once

#include "Vector3.h"
#include "Matrix4.h"
#include <cmath>
#include <algorithm>

namespace BGE {

class Ray {
public:
    Vector3 origin;
    Vector3 direction;
    
    Ray() : origin(0, 0, 0), direction(0, 0, 1) {}
    Ray(const Vector3& origin, const Vector3& direction) 
        : origin(origin), direction(direction.Normalized()) {}
    
    Vector3 GetPoint(float distance) const {
        return origin + direction * distance;
    }
    
    bool IntersectPlane(const Vector3& planeNormal, const Vector3& planePoint, float& t) const {
        float denom = planeNormal.Dot(direction);
        if (fabs(denom) < 0.0001f) return false;
        
        Vector3 p0l0 = planePoint - origin;
        t = p0l0.Dot(planeNormal) / denom;
        return t >= 0;
    }
    
    bool IntersectSphere(const Vector3& center, float radius, float& t) const {
        Vector3 oc = origin - center;
        float a = direction.Dot(direction);
        float b = 2.0f * oc.Dot(direction);
        float c = oc.Dot(oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return false;
        
        float sqrtD = sqrtf(discriminant);
        float t0 = (-b - sqrtD) / (2.0f * a);
        float t1 = (-b + sqrtD) / (2.0f * a);
        
        if (t0 > t1) std::swap(t0, t1);
        
        if (t0 < 0) {
            t0 = t1;
            if (t0 < 0) return false;
        }
        
        t = t0;
        return true;
    }
    
    bool IntersectBox(const Vector3& min, const Vector3& max, float& t) const {
        float tmin = 0.0f;
        float tmax = 1e30f;
        
        for (int i = 0; i < 3; i++) {
            float origin_i = (i == 0) ? origin.x : (i == 1) ? origin.y : origin.z;
            float direction_i = (i == 0) ? direction.x : (i == 1) ? direction.y : direction.z;
            float min_i = (i == 0) ? min.x : (i == 1) ? min.y : min.z;
            float max_i = (i == 0) ? max.x : (i == 1) ? max.y : max.z;
            
            if (fabs(direction_i) < 0.0001f) {
                if (origin_i < min_i || origin_i > max_i) return false;
            } else {
                float ood = 1.0f / direction_i;
                float t1 = (min_i - origin_i) * ood;
                float t2 = (max_i - origin_i) * ood;
                
                if (t1 > t2) std::swap(t1, t2);
                
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                
                if (tmin > tmax) return false;
            }
        }
        
        t = tmin;
        return true;
    }
    
    bool IntersectCylinder(const Vector3& base, const Vector3& axis, float radius, float height, float& t) const {
        Vector3 d = direction;
        Vector3 s = origin - base;
        Vector3 a = axis.Normalized();
        
        float a_dot_d = a.Dot(d);
        float a_dot_s = a.Dot(s);
        
        Vector3 s_perp = s - a * a_dot_s;
        Vector3 d_perp = d - a * a_dot_d;
        
        float A = d_perp.Dot(d_perp);
        float B = 2.0f * s_perp.Dot(d_perp);
        float C = s_perp.Dot(s_perp) - radius * radius;
        
        float discriminant = B * B - 4.0f * A * C;
        if (discriminant < 0) return false;
        
        float sqrtD = sqrtf(discriminant);
        float t0 = (-B - sqrtD) / (2.0f * A);
        float t1 = (-B + sqrtD) / (2.0f * A);
        
        if (t0 > t1) std::swap(t0, t1);
        if (t1 < 0) return false;
        
        float y0 = a_dot_s + t0 * a_dot_d;
        float y1 = a_dot_s + t1 * a_dot_d;
        
        if (y0 < 0) {
            if (y1 < 0) return false;
            t0 = t0 + (t1 - t0) * (0 - y0) / (y1 - y0);
        } else if (y0 > height) {
            if (y1 > height) return false;
            t0 = t0 + (t1 - t0) * (height - y0) / (y1 - y0);
        }
        
        if (t0 < 0) return false;
        
        t = t0;
        return true;
    }
    
    bool IntersectTorus(const Vector3& center, const Vector3& axis, float majorRadius, float minorRadius, float& t) const {
        // Simplified torus intersection using sampling
        // This is not as accurate as analytical solution but works for gizmo purposes
        Vector3 a = axis.Normalized();
        
        float minT = 1e30f;
        bool hit = false;
        
        // Sample along the ray to find intersection
        for (int i = 0; i < 100; i++) {
            float test_t = i * 0.1f;
            Vector3 p = GetPoint(test_t);
            Vector3 toCenter = p - center;
            Vector3 onAxis = a * a.Dot(toCenter);
            Vector3 toRing = toCenter - onAxis;
            float distToRing = toRing.Length();
            
            if (distToRing > 0.001f) {
                Vector3 ringPoint = center + onAxis + toRing.Normalized() * majorRadius;
                float dist = (p - ringPoint).Length();
                
                if (dist < minorRadius && test_t < minT) {
                    minT = test_t;
                    hit = true;
                }
            }
        }
        
        if (hit) {
            t = minT;
            return true;
        }
        
        return false;
    }
    
    static Ray ScreenPointToRay(float screenX, float screenY, float screenWidth, float screenHeight,
                                const Matrix4& viewMatrix, const Matrix4& projMatrix) {
        float x = (2.0f * screenX) / screenWidth - 1.0f;
        float y = 1.0f - (2.0f * screenY) / screenHeight;
        
        Matrix4 viewProjInv = (projMatrix * viewMatrix).Inverse();
        
        Vector3 nearPoint(x, y, -1.0f);
        Vector3 farPoint(x, y, 1.0f);
        
        nearPoint = viewProjInv.TransformPoint(nearPoint);
        farPoint = viewProjInv.TransformPoint(farPoint);
        
        return Ray(nearPoint, (farPoint - nearPoint).Normalized());
    }
};

} // namespace BGE