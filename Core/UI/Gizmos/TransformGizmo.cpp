#include "TransformGizmo.h"
#include "../../Math/Vector2.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace BGE {

TransformGizmo::TransformGizmo() {
    m_rotation = Quaternion::Identity();
}

void TransformGizmo::SetTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
    m_position = position;
    m_rotation = rotation;
    m_scale = scale;
}

void TransformGizmo::GetTransform(Vector3& position, Quaternion& rotation, Vector3& scale) const {
    position = m_position;
    rotation = m_rotation;
    scale = m_scale;
}

void TransformGizmo::SetSnapping(bool enabled, float translationSnap, float rotationSnap, float scaleSnap) {
    m_snapping = enabled;
    m_translationSnap = translationSnap;
    m_rotationSnap = rotationSnap * (3.14159265f / 180.0f); // Convert to radians
    m_scaleSnap = scaleSnap;
}

void TransformGizmo::Render(ImDrawList* drawList, const Matrix4& viewMatrix, const Matrix4& projMatrix,
                           ImVec2 viewportPos, ImVec2 viewportSize, const Vector3& cameraPos) {
    float cameraDistance = (m_position - cameraPos).Length();
    // Use a fixed scale for now to match Unity's behavior
    m_gizmoScale = 1.0f;
    
    switch (m_renderer.GetMode()) {
        case GizmoMode::Translate:
            m_renderer.RenderTranslationGizmo(drawList, m_position, m_rotation, cameraDistance,
                                            viewMatrix, projMatrix, viewportPos, viewportSize);
            break;
        case GizmoMode::Rotate:
            m_renderer.RenderRotationGizmo(drawList, m_position, m_rotation, cameraDistance,
                                         viewMatrix, projMatrix, viewportPos, viewportSize);
            break;
        case GizmoMode::Scale:
            m_renderer.RenderScaleGizmo(drawList, m_position, m_rotation, cameraDistance,
                                       viewMatrix, projMatrix, viewportPos, viewportSize);
            break;
    }
}

bool TransformGizmo::HandleMouseInput(const Ray& mouseRay, bool mouseDown, bool mouseDragged,
                                     const Vector3& cameraPosition, const Vector3& cameraForward) {
    // ImGuiIO& io = ImGui::GetIO();
    // bool shiftHeld = io.KeyShift;
    
    if (!mouseDown && !mouseDragged) {
        GizmoAxis hoveredAxis = GetAxisUnderMouse(mouseRay, cameraPosition);
        m_renderer.SetHighlightedAxis(hoveredAxis);
        
        if (m_activeAxis != GizmoAxis::None) {
            EndDrag();
        }
        return false;
    }
    
    if (mouseDown && m_activeAxis == GizmoAxis::None) {
        GizmoAxis clickedAxis = GetAxisUnderMouse(mouseRay, cameraPosition);
        if (clickedAxis != GizmoAxis::None) {
            StartDrag(clickedAxis, mouseRay, cameraPosition);
            return true;
        }
    }
    
    if (mouseDragged && m_activeAxis != GizmoAxis::None) {
        UpdateDrag(mouseRay, cameraPosition, cameraForward);
        return true;
    }
    
    return false;
}

GizmoAxis TransformGizmo::GetAxisUnderMouse(const Ray& ray, const Vector3& /*cameraPosition*/) {
    float closestT = 1e30f;
    GizmoAxis closestAxis = GizmoAxis::None;
    
    Quaternion rot = (m_renderer.GetSpace() == GizmoSpace::Local) ? m_rotation : Quaternion::Identity();
    Vector3 xAxis = rot * Vector3(1, 0, 0);
    Vector3 yAxis = rot * Vector3(0, 1, 0);
    Vector3 zAxis = rot * Vector3(0, 0, 1);
    
    float t;
    
    switch (m_renderer.GetMode()) {
        case GizmoMode::Translate:
            if (TestTranslationAxis(ray, xAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::X;
            }
            if (TestTranslationAxis(ray, yAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::Y;
            }
            if (TestTranslationAxis(ray, zAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::Z;
            }
            
            if (TestPlaneHandle(ray, xAxis, yAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::XY;
            }
            if (TestPlaneHandle(ray, xAxis, zAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::XZ;
            }
            if (TestPlaneHandle(ray, yAxis, zAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::YZ;
            }
            break;
            
        case GizmoMode::Rotate:
            if (TestRotationAxis(ray, xAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::X;
            }
            if (TestRotationAxis(ray, yAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::Y;
            }
            if (TestRotationAxis(ray, zAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::Z;
            }
            
            if (ray.IntersectSphere(m_position, m_gizmoScale * 1.2f, t) && t < closestT) {
                closestAxis = GizmoAxis::Screen;
            }
            break;
            
        case GizmoMode::Scale:
            if (TestScaleAxis(ray, xAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::X;
            }
            if (TestScaleAxis(ray, yAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::Y;
            }
            if (TestScaleAxis(ray, zAxis, t) && t < closestT) {
                closestT = t;
                closestAxis = GizmoAxis::Z;
            }
            
            if (ray.IntersectBox(m_position - Vector3(m_gizmoScale * 0.1f),
                               m_position + Vector3(m_gizmoScale * 0.1f), t) && t < closestT) {
                closestAxis = GizmoAxis::All;
            }
            break;
    }
    
    return closestAxis;
}

bool TransformGizmo::TestTranslationAxis(const Ray& ray, const Vector3& axis, float& t) {
    Vector3 end = m_position + axis * m_gizmoScale;
    return ray.IntersectCylinder(m_position, axis, m_gizmoScale * 0.05f, m_gizmoScale, t);
}

bool TransformGizmo::TestRotationAxis(const Ray& ray, const Vector3& axis, float& t) {
    return ray.IntersectTorus(m_position, axis, m_gizmoScale, m_gizmoScale * 0.05f, t);
}

bool TransformGizmo::TestScaleAxis(const Ray& ray, const Vector3& axis, float& t) {
    Vector3 end = m_position + axis * m_gizmoScale;
    bool hit = ray.IntersectCylinder(m_position, axis, m_gizmoScale * 0.05f, m_gizmoScale, t);
    if (!hit) {
        hit = ray.IntersectBox(end - Vector3(m_gizmoScale * 0.05f),
                              end + Vector3(m_gizmoScale * 0.05f), t);
    }
    return hit;
}

bool TransformGizmo::TestPlaneHandle(const Ray& ray, const Vector3& normal1, const Vector3& normal2, float& t) {
    Vector3 planeNormal = normal1.Cross(normal2);
    float planeSize = m_gizmoScale * 0.25f;
    
    if (ray.IntersectPlane(planeNormal, m_position, t)) {
        Vector3 hitPoint = ray.GetPoint(t);
        Vector3 localPoint = hitPoint - m_position;
        
        float proj1 = localPoint.Dot(normal1);
        float proj2 = localPoint.Dot(normal2);
        
        return proj1 >= 0 && proj1 <= planeSize && proj2 >= 0 && proj2 <= planeSize;
    }
    return false;
}

void TransformGizmo::StartDrag(GizmoAxis axis, const Ray& ray, const Vector3& /*cameraPosition*/) {
    m_activeAxis = axis;
    m_initialPosition = m_position;
    m_initialRotation = m_rotation;
    m_initialScale = m_scale;
    
    float t;
    switch (m_renderer.GetMode()) {
        case GizmoMode::Translate:
            if ((int)axis & (int)GizmoAxis::X || (int)axis & (int)GizmoAxis::Y || (int)axis & (int)GizmoAxis::Z) {
                Vector3 axisDir = GetAxisDirection(axis);
                if (ray.IntersectPlane(axisDir, m_position, t)) {
                    m_dragStart = ray.GetPoint(t);
                    m_dragOffset = m_position - m_dragStart;
                }
            } else {
                Vector3 planeNormal = GetPlaneNormal(axis);
                if (ray.IntersectPlane(planeNormal, m_position, t)) {
                    m_dragStart = ray.GetPoint(t);
                    m_dragOffset = Vector3(0, 0, 0);
                }
            }
            break;
            
        case GizmoMode::Rotate:
            if (axis == GizmoAxis::Screen) {
                m_dragStart = Vector3(ImGui::GetMousePos().x, ImGui::GetMousePos().y, 0);
            } else {
                Vector3 axisDir = GetAxisDirection(axis);
                if (ray.IntersectPlane(axisDir, m_position, t)) {
                    Vector3 hitPoint = ray.GetPoint(t) - m_position;
                    m_dragAngleStart = atan2f(hitPoint.y, hitPoint.x);
                }
            }
            break;
            
        case GizmoMode::Scale:
            m_dragStart = Vector3(ImGui::GetMousePos().x, ImGui::GetMousePos().y, 0);
            break;
    }
}

void TransformGizmo::UpdateDrag(const Ray& ray, const Vector3& /*cameraPosition*/, const Vector3& cameraForward) {
    ImGuiIO& io = ImGui::GetIO();
    bool shiftHeld = io.KeyShift;
    
    switch (m_renderer.GetMode()) {
        case GizmoMode::Translate: {
            float t;
            Vector3 newPos = m_initialPosition;
            
            if ((int)m_activeAxis & (int)GizmoAxis::X || 
                (int)m_activeAxis & (int)GizmoAxis::Y || 
                (int)m_activeAxis & (int)GizmoAxis::Z) {
                Vector3 axisDir = GetAxisDirection(m_activeAxis);
                Vector3 planeNormal = cameraForward.Cross(axisDir).Normalized();
                if (fabs(planeNormal.Length()) < 0.01f) {
                    planeNormal = Vector3(0, 1, 0).Cross(axisDir).Normalized();
                }
                
                if (ray.IntersectPlane(planeNormal, m_position, t)) {
                    Vector3 hitPoint = ray.GetPoint(t);
                    Vector3 delta = ProjectPointOntoAxis(hitPoint - m_dragStart, axisDir);
                    newPos = m_initialPosition + delta;
                }
            } else {
                Vector3 planeNormal = GetPlaneNormal(m_activeAxis);
                if (ray.IntersectPlane(planeNormal, m_position, t)) {
                    Vector3 hitPoint = ray.GetPoint(t);
                    newPos = m_initialPosition + (hitPoint - m_dragStart);
                }
            }
            
            if (m_snapping || shiftHeld) {
                newPos = ApplySnap(newPos, m_translationSnap);
            }
            
            m_position = newPos;
            break;
        }
            
        case GizmoMode::Rotate: {
            if (m_activeAxis == GizmoAxis::Screen) {
                Vector2 currentMouse(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
                Vector2 startMouse(m_dragStart.x, m_dragStart.y);
                Vector2 center(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
                              ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f);
                
                float angle1 = atan2f(startMouse.y - center.y, startMouse.x - center.x);
                float angle2 = atan2f(currentMouse.y - center.y, currentMouse.x - center.x);
                float deltaAngle = angle2 - angle1;
                
                if (m_snapping || shiftHeld) {
                    deltaAngle = ApplySnapAngle(deltaAngle, m_rotationSnap);
                }
                
                Quaternion rot = Quaternion::FromAxisAngle(cameraForward, deltaAngle);
                m_rotation = rot * m_initialRotation;
            } else {
                float t;
                Vector3 axisDir = GetAxisDirection(m_activeAxis);
                if (ray.IntersectPlane(axisDir, m_position, t)) {
                    Vector3 hitPoint = ray.GetPoint(t) - m_position;
                    float angle = atan2f(hitPoint.y, hitPoint.x);
                    float deltaAngle = angle - m_dragAngleStart;
                    
                    if (m_snapping || shiftHeld) {
                        deltaAngle = ApplySnapAngle(deltaAngle, m_rotationSnap);
                    }
                    
                    Quaternion rot = Quaternion::FromAxisAngle(axisDir, deltaAngle);
                    m_rotation = rot * m_initialRotation;
                }
            }
            break;
        }
            
        case GizmoMode::Scale: {
            Vector2 currentMouse(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
            Vector2 startMouse(m_dragStart.x, m_dragStart.y);
            float delta = (currentMouse.x - startMouse.x + currentMouse.y - startMouse.y) * 0.01f;
            
            if (m_activeAxis == GizmoAxis::All) {
                Vector3 scaleChange(1.0f + delta, 1.0f + delta, 1.0f + delta);
                m_scale = Vector3(m_initialScale.x * scaleChange.x,
                                 m_initialScale.y * scaleChange.y,
                                 m_initialScale.z * scaleChange.z);
            } else {
                Vector3 scaleChange(1.0f, 1.0f, 1.0f);
                if (m_activeAxis == GizmoAxis::X) scaleChange.x = 1.0f + delta;
                if (m_activeAxis == GizmoAxis::Y) scaleChange.y = 1.0f + delta;
                if (m_activeAxis == GizmoAxis::Z) scaleChange.z = 1.0f + delta;
                m_scale = Vector3(m_initialScale.x * scaleChange.x,
                                 m_initialScale.y * scaleChange.y,
                                 m_initialScale.z * scaleChange.z);
            }
            
            if (m_snapping || shiftHeld) {
                m_scale = ApplySnap(m_scale, m_scaleSnap);
            }
            
            m_scale = Vector3(std::max(0.01f, m_scale.x),
                             std::max(0.01f, m_scale.y),
                             std::max(0.01f, m_scale.z));
            break;
        }
    }
    
    if (m_callback) {
        m_callback(m_position, m_rotation, m_scale);
    }
}

void TransformGizmo::EndDrag() {
    m_activeAxis = GizmoAxis::None;
}

Vector3 TransformGizmo::ApplySnap(const Vector3& value, float snap) {
    return Vector3(
        roundf(value.x / snap) * snap,
        roundf(value.y / snap) * snap,
        roundf(value.z / snap) * snap
    );
}

float TransformGizmo::ApplySnapAngle(float angle, float snap) {
    return roundf(angle / snap) * snap;
}

Vector3 TransformGizmo::ProjectPointOntoAxis(const Vector3& point, const Vector3& axis) {
    return axis * point.Dot(axis);
}

Vector3 TransformGizmo::ProjectPointOntoPlane(const Vector3& point, const Vector3& planeNormal, const Vector3& planePoint) {
    Vector3 v = point - planePoint;
    float dist = v.Dot(planeNormal);
    return point - planeNormal * dist;
}

Vector3 TransformGizmo::GetAxisDirection(GizmoAxis axis) {
    Quaternion rot = (m_renderer.GetSpace() == GizmoSpace::Local) ? m_rotation : Quaternion::Identity();
    
    switch (axis) {
        case GizmoAxis::X: return rot * Vector3(1, 0, 0);
        case GizmoAxis::Y: return rot * Vector3(0, 1, 0);
        case GizmoAxis::Z: return rot * Vector3(0, 0, 1);
        default: return Vector3(0, 0, 0);
    }
}

Vector3 TransformGizmo::GetPlaneNormal(GizmoAxis axis) {
    Quaternion rot = (m_renderer.GetSpace() == GizmoSpace::Local) ? m_rotation : Quaternion::Identity();
    
    switch (axis) {
        case GizmoAxis::XY: return rot * Vector3(0, 0, 1);
        case GizmoAxis::XZ: return rot * Vector3(0, 1, 0);
        case GizmoAxis::YZ: return rot * Vector3(1, 0, 0);
        default: return Vector3(0, 1, 0);
    }
}

} // namespace BGE