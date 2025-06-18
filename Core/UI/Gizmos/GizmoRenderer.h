#pragma once

#include "../../Math/Vector3.h"
#include "../../Math/Matrix4.h"
#include "../../Math/Quaternion.h"
#include <imgui.h>

namespace BGE {

enum class GizmoMode {
    Translate,
    Rotate,
    Scale
};

enum class GizmoSpace {
    World,
    Local
};

enum class GizmoAxis {
    None = 0,
    X = 1 << 0,
    Y = 1 << 1,
    Z = 1 << 2,
    XY = X | Y,
    XZ = X | Z,
    YZ = Y | Z,
    Screen = 1 << 3,
    All = X | Y | Z
};

class GizmoRenderer {
public:
    GizmoRenderer();
    ~GizmoRenderer() = default;
    
    void SetMode(GizmoMode mode) { m_mode = mode; }
    GizmoMode GetMode() const { return m_mode; }
    
    void SetSpace(GizmoSpace space) { m_space = space; }
    GizmoSpace GetSpace() const { return m_space; }
    
    void SetHighlightedAxis(GizmoAxis axis) { m_highlightedAxis = axis; }
    GizmoAxis GetHighlightedAxis() const { return m_highlightedAxis; }
    
    void SetGizmoSize(float size) { m_gizmoSize = size; }
    float GetGizmoSize() const { return m_gizmoSize; }
    
    void RenderTranslationGizmo(ImDrawList* drawList, const Vector3& position, 
                               const Quaternion& rotation, float cameraDistance,
                               const Matrix4& viewMatrix, const Matrix4& projMatrix,
                               ImVec2 viewportPos, ImVec2 viewportSize);
    
    void RenderRotationGizmo(ImDrawList* drawList, const Vector3& position,
                            const Quaternion& rotation, float cameraDistance,
                            const Matrix4& viewMatrix, const Matrix4& projMatrix,
                            ImVec2 viewportPos, ImVec2 viewportSize);
    
    void RenderScaleGizmo(ImDrawList* drawList, const Vector3& position,
                         const Quaternion& rotation, float cameraDistance,
                         const Matrix4& viewMatrix, const Matrix4& projMatrix,
                         ImVec2 viewportPos, ImVec2 viewportSize);
    
private:
    Vector3 WorldToScreen(const Vector3& worldPos, const Matrix4& viewMatrix,
                         const Matrix4& projMatrix, ImVec2 viewportPos, ImVec2 viewportSize);
    
    void DrawArrow(ImDrawList* drawList, const Vector3& start, const Vector3& end,
                  ImU32 color, float thickness, const Matrix4& viewMatrix,
                  const Matrix4& projMatrix, ImVec2 viewportPos, ImVec2 viewportSize);
    
    void DrawCircle(ImDrawList* drawList, const Vector3& center, const Vector3& normal,
                   float radius, ImU32 color, float thickness, int segments,
                   const Matrix4& viewMatrix, const Matrix4& projMatrix,
                   ImVec2 viewportPos, ImVec2 viewportSize);
    
    void DrawBox(ImDrawList* drawList, const Vector3& center, const Vector3& size,
                const Quaternion& rotation, ImU32 color, float thickness,
                const Matrix4& viewMatrix, const Matrix4& projMatrix,
                ImVec2 viewportPos, ImVec2 viewportSize);
    
    void DrawPlaneHandle(ImDrawList* drawList, const Vector3& center,
                        const Vector3& normal1, const Vector3& normal2,
                        float size, ImU32 color, const Matrix4& viewMatrix,
                        const Matrix4& projMatrix, ImVec2 viewportPos, ImVec2 viewportSize);
    
    ImU32 GetAxisColor(GizmoAxis axis, bool highlighted = false);
    
    GizmoMode m_mode = GizmoMode::Translate;
    GizmoSpace m_space = GizmoSpace::World;
    GizmoAxis m_highlightedAxis = GizmoAxis::None;
    float m_gizmoSize = 80.0f; // Screen space size in pixels
    
    // Unity-style gizmo colors
    ImU32 m_colorX = IM_COL32(221, 56, 53, 255);      // Red (Unity's red)
    ImU32 m_colorY = IM_COL32(130, 214, 29, 255);     // Green (Unity's green)
    ImU32 m_colorZ = IM_COL32(50, 133, 253, 255);     // Blue (Unity's blue)
    ImU32 m_colorHighlight = IM_COL32(255, 235, 4, 255); // Yellow highlight
    ImU32 m_colorScreen = IM_COL32(255, 255, 255, 200); // White screen rotation
    ImU32 m_colorPlane = IM_COL32(255, 235, 4, 100);  // Yellow plane with alpha
};

} // namespace BGE