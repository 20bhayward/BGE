#pragma once

#include "GizmoRenderer.h"
#include "../../Math/Ray.h"
#include <functional>

namespace BGE {

class TransformGizmo {
public:
    using TransformCallback = std::function<void(const Vector3&, const Quaternion&, const Vector3&)>;
    
    TransformGizmo();
    ~TransformGizmo() = default;
    
    void SetMode(GizmoMode mode) { m_renderer.SetMode(mode); }
    GizmoMode GetMode() const { return m_renderer.GetMode(); }
    
    void SetSpace(GizmoSpace space) { m_renderer.SetSpace(space); }
    GizmoSpace GetSpace() const { return m_renderer.GetSpace(); }
    
    void SetTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);
    void GetTransform(Vector3& position, Quaternion& rotation, Vector3& scale) const;
    
    void SetCallback(TransformCallback callback) { m_callback = callback; }
    
    void SetSnapping(bool enabled, float translationSnap = 1.0f, float rotationSnap = 15.0f, float scaleSnap = 0.1f);
    bool IsSnapping() const { return m_snapping; }
    
    void Render(ImDrawList* drawList, const Matrix4& viewMatrix, const Matrix4& projMatrix,
               ImVec2 viewportPos, ImVec2 viewportSize, const Vector3& cameraPos);
    
    bool HandleMouseInput(const Ray& mouseRay, bool mouseDown, bool mouseDragged,
                         const Vector3& cameraPosition, const Vector3& cameraForward);
    
    bool IsActive() const { return m_activeAxis != GizmoAxis::None; }
    GizmoAxis GetHighlightedAxis() const { return m_renderer.GetHighlightedAxis(); }
    
private:
    GizmoAxis GetAxisUnderMouse(const Ray& ray, const Vector3& cameraPosition);
    
    bool TestTranslationAxis(const Ray& ray, const Vector3& axis, float& t);
    bool TestRotationAxis(const Ray& ray, const Vector3& axis, float& t);
    bool TestScaleAxis(const Ray& ray, const Vector3& axis, float& t);
    bool TestPlaneHandle(const Ray& ray, const Vector3& normal1, const Vector3& normal2, float& t);
    
    void StartDrag(GizmoAxis axis, const Ray& ray, const Vector3& cameraPosition);
    void UpdateDrag(const Ray& ray, const Vector3& cameraPosition, const Vector3& cameraForward);
    void EndDrag();
    
    Vector3 ApplySnap(const Vector3& value, float snap);
    float ApplySnapAngle(float angle, float snap);
    
    Vector3 ProjectPointOntoAxis(const Vector3& point, const Vector3& axis);
    Vector3 ProjectPointOntoPlane(const Vector3& point, const Vector3& planeNormal, const Vector3& planePoint);
    
    GizmoRenderer m_renderer;
    TransformCallback m_callback;
    
    Vector3 m_position{0, 0, 0};
    Quaternion m_rotation;
    Vector3 m_scale{1, 1, 1};
    
    Vector3 m_initialPosition;
    Quaternion m_initialRotation;
    Vector3 m_initialScale;
    
    GizmoAxis m_activeAxis = GizmoAxis::None;
    Vector3 m_dragStart;
    Vector3 m_dragOffset;
    float m_dragAngleStart = 0.0f;
    
    bool m_snapping = false;
    float m_translationSnap = 1.0f;
    float m_rotationSnap = 15.0f;
    float m_scaleSnap = 0.1f;
    
    float m_gizmoScale = 1.0f;
    
    // Helper functions
    Vector3 GetAxisDirection(GizmoAxis axis);
    Vector3 GetPlaneNormal(GizmoAxis axis);
};

} // namespace BGE