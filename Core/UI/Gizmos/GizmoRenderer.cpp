#include "GizmoRenderer.h"
#include "../../Math/Vector4.h"
#include <cmath>
#include <vector>

namespace BGE {

GizmoRenderer::GizmoRenderer() {
}

Vector3 GizmoRenderer::WorldToScreen(const Vector3& worldPos, const Matrix4& viewMatrix,
                                    const Matrix4& projMatrix, ImVec2 viewportPos, ImVec2 viewportSize) {
    Vector4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    pos = viewMatrix * pos;
    pos = projMatrix * pos;
    
    if (pos.w != 0.0f) {
        pos.x /= pos.w;
        pos.y /= pos.w;
        pos.z /= pos.w;
    }
    
    float x = (pos.x + 1.0f) * 0.5f * viewportSize.x + viewportPos.x;
    float y = (1.0f - pos.y) * 0.5f * viewportSize.y + viewportPos.y;
    
    return Vector3(x, y, pos.z);
}

void GizmoRenderer::DrawArrow(ImDrawList* drawList, const Vector3& start, const Vector3& end,
                             ImU32 color, float thickness, const Matrix4& viewMatrix,
                             const Matrix4& projMatrix, ImVec2 viewportPos, ImVec2 viewportSize) {
    Vector3 screenStart = WorldToScreen(start, viewMatrix, projMatrix, viewportPos, viewportSize);
    Vector3 screenEnd = WorldToScreen(end, viewMatrix, projMatrix, viewportPos, viewportSize);
    
    if (screenStart.z < 0 || screenEnd.z < 0 || screenStart.z > 1 || screenEnd.z > 1) return;
    
    // Draw main line
    drawList->AddLine(ImVec2(screenStart.x, screenStart.y), ImVec2(screenEnd.x, screenEnd.y), color, thickness);
    
    // Unity-style cone arrowhead
    Vector3 dir = (end - start).Normalized();
    float arrowLength = (end - start).Length() * 0.15f;
    float arrowRadius = arrowLength * 0.5f;
    
    // Create cone base points
    Vector3 arrowBase = end - dir * arrowLength;
    
    // Get perpendicular vectors
    Vector3 up = Vector3(0, 1, 0);
    if (fabs(dir.Dot(up)) > 0.9f) {
        up = Vector3(1, 0, 0);
    }
    Vector3 right = dir.Cross(up).Normalized();
    Vector3 forward = dir.Cross(right).Normalized();
    
    // Draw cone as triangle fan
    const int segments = 8;
    std::vector<ImVec2> conePoints;
    
    // Add tip
    Vector3 tipScreen = WorldToScreen(end, viewMatrix, projMatrix, viewportPos, viewportSize);
    if (tipScreen.z >= 0 && tipScreen.z <= 1) {
        conePoints.push_back(ImVec2(tipScreen.x, tipScreen.y));
        
        // Add base circle points
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159265f;
            Vector3 basePoint = arrowBase + (right * cosf(angle) + forward * sinf(angle)) * arrowRadius;
            Vector3 baseScreen = WorldToScreen(basePoint, viewMatrix, projMatrix, viewportPos, viewportSize);
            
            if (baseScreen.z >= 0 && baseScreen.z <= 1) {
                conePoints.push_back(ImVec2(baseScreen.x, baseScreen.y));
            }
        }
        
        // Draw filled cone
        if (conePoints.size() >= 3) {
            for (size_t i = 1; i < conePoints.size() - 1; i++) {
                ImVec2 triangle[3] = { conePoints[0], conePoints[i], conePoints[i + 1] };
                drawList->AddConvexPolyFilled(triangle, 3, color);
            }
        }
    }
}

void GizmoRenderer::DrawCircle(ImDrawList* drawList, const Vector3& center, const Vector3& normal,
                              float radius, ImU32 color, float thickness, int segments,
                              const Matrix4& viewMatrix, const Matrix4& projMatrix,
                              ImVec2 viewportPos, ImVec2 viewportSize) {
    Vector3 tangent = Vector3(1, 0, 0);
    if (fabs(normal.Dot(tangent)) > 0.99f) {
        tangent = Vector3(0, 1, 0);
    }
    
    Vector3 binormal = normal.Cross(tangent).Normalized();
    tangent = binormal.Cross(normal).Normalized();
    
    std::vector<ImVec2> points;
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159265f;
        Vector3 point = center + (tangent * cosf(angle) + binormal * sinf(angle)) * radius;
        Vector3 screenPoint = WorldToScreen(point, viewMatrix, projMatrix, viewportPos, viewportSize);
        
        if (screenPoint.z >= 0 && screenPoint.z <= 1) {
            points.push_back(ImVec2(screenPoint.x, screenPoint.y));
        }
    }
    
    if (points.size() > 1) {
        for (size_t i = 0; i < points.size() - 1; i++) {
            drawList->AddLine(points[i], points[i + 1], color, thickness);
        }
    }
}

void GizmoRenderer::DrawBox(ImDrawList* drawList, const Vector3& center, const Vector3& size,
                           const Quaternion& rotation, ImU32 color, float thickness,
                           const Matrix4& viewMatrix, const Matrix4& projMatrix,
                           ImVec2 viewportPos, ImVec2 viewportSize) {
    Vector3 halfSize = size * 0.5f;
    Vector3 corners[8] = {
        Vector3(-halfSize.x, -halfSize.y, -halfSize.z),
        Vector3( halfSize.x, -halfSize.y, -halfSize.z),
        Vector3( halfSize.x,  halfSize.y, -halfSize.z),
        Vector3(-halfSize.x,  halfSize.y, -halfSize.z),
        Vector3(-halfSize.x, -halfSize.y,  halfSize.z),
        Vector3( halfSize.x, -halfSize.y,  halfSize.z),
        Vector3( halfSize.x,  halfSize.y,  halfSize.z),
        Vector3(-halfSize.x,  halfSize.y,  halfSize.z)
    };
    
    for (int i = 0; i < 8; i++) {
        corners[i] = rotation * corners[i] + center;
    }
    
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    
    for (int i = 0; i < 12; i++) {
        Vector3 screen1 = WorldToScreen(corners[edges[i][0]], viewMatrix, projMatrix, viewportPos, viewportSize);
        Vector3 screen2 = WorldToScreen(corners[edges[i][1]], viewMatrix, projMatrix, viewportPos, viewportSize);
        
        if (screen1.z >= 0 && screen1.z <= 1 && screen2.z >= 0 && screen2.z <= 1) {
            drawList->AddLine(ImVec2(screen1.x, screen1.y), ImVec2(screen2.x, screen2.y), color, thickness);
        }
    }
}

void GizmoRenderer::DrawPlaneHandle(ImDrawList* drawList, const Vector3& center,
                                   const Vector3& normal1, const Vector3& normal2,
                                   float size, ImU32 color, const Matrix4& viewMatrix,
                                   const Matrix4& projMatrix, ImVec2 viewportPos, ImVec2 viewportSize) {
    Vector3 p1 = center;
    Vector3 p2 = center + normal1 * size;
    Vector3 p3 = center + normal1 * size + normal2 * size;
    Vector3 p4 = center + normal2 * size;
    
    Vector3 screen1 = WorldToScreen(p1, viewMatrix, projMatrix, viewportPos, viewportSize);
    Vector3 screen2 = WorldToScreen(p2, viewMatrix, projMatrix, viewportPos, viewportSize);
    Vector3 screen3 = WorldToScreen(p3, viewMatrix, projMatrix, viewportPos, viewportSize);
    Vector3 screen4 = WorldToScreen(p4, viewMatrix, projMatrix, viewportPos, viewportSize);
    
    if (screen1.z >= 0 && screen1.z <= 1 && screen2.z >= 0 && screen2.z <= 1 &&
        screen3.z >= 0 && screen3.z <= 1 && screen4.z >= 0 && screen4.z <= 1) {
        ImVec2 points[4] = {
            ImVec2(screen1.x, screen1.y),
            ImVec2(screen2.x, screen2.y),
            ImVec2(screen3.x, screen3.y),
            ImVec2(screen4.x, screen4.y)
        };
        drawList->AddConvexPolyFilled(points, 4, color);
    }
}

ImU32 GizmoRenderer::GetAxisColor(GizmoAxis axis, bool highlighted) {
    if (highlighted || axis == m_highlightedAxis) {
        return m_colorHighlight;
    }
    
    switch (axis) {
        case GizmoAxis::X: return m_colorX;
        case GizmoAxis::Y: return m_colorY;
        case GizmoAxis::Z: return m_colorZ;
        case GizmoAxis::Screen: return m_colorScreen;
        default: return m_colorPlane;
    }
}

void GizmoRenderer::RenderTranslationGizmo(ImDrawList* drawList, const Vector3& position,
                                          const Quaternion& rotation, float /*cameraDistance*/,
                                          const Matrix4& viewMatrix, const Matrix4& projMatrix,
                                          ImVec2 viewportPos, ImVec2 viewportSize) {
    // Unity-style sizing: constant screen size regardless of distance
    float scale = m_gizmoSize * 0.15f;
    
    Quaternion rot = (m_space == GizmoSpace::Local) ? rotation : Quaternion::Identity();
    Vector3 xAxis = rot * Vector3(1, 0, 0);
    Vector3 yAxis = rot * Vector3(0, 1, 0);
    Vector3 zAxis = rot * Vector3(0, 0, 1);
    
    // Draw thicker axis lines
    float lineThickness = 4.0f;
    
    // Draw axis lines
    Vector3 xEnd = position + xAxis * scale;
    Vector3 yEnd = position + yAxis * scale;
    Vector3 zEnd = position + zAxis * scale;
    
    // X axis (Red)
    DrawArrow(drawList, position, xEnd, 
              GetAxisColor(GizmoAxis::X, m_highlightedAxis == GizmoAxis::X), lineThickness,
              viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Y axis (Green)
    DrawArrow(drawList, position, yEnd,
              GetAxisColor(GizmoAxis::Y, m_highlightedAxis == GizmoAxis::Y), lineThickness,
              viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Z axis (Blue)
    DrawArrow(drawList, position, zEnd,
              GetAxisColor(GizmoAxis::Z, m_highlightedAxis == GizmoAxis::Z), lineThickness,
              viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Draw plane handles (Unity-style squares at origin)
    float planeSize = scale * 0.2f;
    float planeAlpha = 80;
    
    // Always draw plane handles, highlighted when active
    // XY plane (Blue square)
    ImU32 xyColor = (m_highlightedAxis == GizmoAxis::XY) ? m_colorHighlight : IM_COL32(50, 133, 253, planeAlpha);
    DrawPlaneHandle(drawList, position, xAxis * planeSize, yAxis * planeSize,
                   1.0f, xyColor, viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // XZ plane (Green square)
    ImU32 xzColor = (m_highlightedAxis == GizmoAxis::XZ) ? m_colorHighlight : IM_COL32(130, 214, 29, planeAlpha);
    DrawPlaneHandle(drawList, position, xAxis * planeSize, zAxis * planeSize,
                   1.0f, xzColor, viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // YZ plane (Red square)
    ImU32 yzColor = (m_highlightedAxis == GizmoAxis::YZ) ? m_colorHighlight : IM_COL32(221, 56, 53, planeAlpha);
    DrawPlaneHandle(drawList, position, yAxis * planeSize, zAxis * planeSize,
                   1.0f, yzColor, viewMatrix, projMatrix, viewportPos, viewportSize);
}

void GizmoRenderer::RenderRotationGizmo(ImDrawList* drawList, const Vector3& position,
                                       const Quaternion& rotation, float /*cameraDistance*/,
                                       const Matrix4& viewMatrix, const Matrix4& projMatrix,
                                       ImVec2 viewportPos, ImVec2 viewportSize) {
    // Unity-style constant screen size
    float scale = m_gizmoSize * 0.2f;
    
    Quaternion rot = (m_space == GizmoSpace::Local) ? rotation : Quaternion::Identity();
    Vector3 xAxis = rot * Vector3(1, 0, 0);
    Vector3 yAxis = rot * Vector3(0, 1, 0);
    Vector3 zAxis = rot * Vector3(0, 0, 1);
    
    // Draw rotation circles with thicker lines
    float thickness = 4.0f;
    int segments = 48; // More segments for smoother circles
    
    // X axis rotation (Red)
    DrawCircle(drawList, position, xAxis, scale,
               GetAxisColor(GizmoAxis::X, m_highlightedAxis == GizmoAxis::X), thickness, segments,
               viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Y axis rotation (Green)
    DrawCircle(drawList, position, yAxis, scale,
               GetAxisColor(GizmoAxis::Y, m_highlightedAxis == GizmoAxis::Y), thickness, segments,
               viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Z axis rotation (Blue)
    DrawCircle(drawList, position, zAxis, scale,
               GetAxisColor(GizmoAxis::Z, m_highlightedAxis == GizmoAxis::Z), thickness, segments,
               viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Screen space rotation (White/Gray outer ring)
    Vector3 camPos = Vector3(-viewMatrix(0, 3), -viewMatrix(1, 3), -viewMatrix(2, 3));
    Vector3 viewDir = (position - camPos).Normalized();
    ImU32 screenColor = (m_highlightedAxis == GizmoAxis::Screen) ? m_colorHighlight : IM_COL32(200, 200, 200, 255);
    DrawCircle(drawList, position, viewDir, scale * 1.2f,
               screenColor, thickness - 1.0f, segments,
               viewMatrix, projMatrix, viewportPos, viewportSize);
}

void GizmoRenderer::RenderScaleGizmo(ImDrawList* drawList, const Vector3& position,
                                    const Quaternion& rotation, float /*cameraDistance*/,
                                    const Matrix4& viewMatrix, const Matrix4& projMatrix,
                                    ImVec2 viewportPos, ImVec2 viewportSize) {
    // Unity-style constant screen size
    float scale = m_gizmoSize * 0.15f;
    
    Quaternion rot = (m_space == GizmoSpace::Local) ? rotation : Quaternion::Identity();
    Vector3 xAxis = rot * Vector3(1, 0, 0);
    Vector3 yAxis = rot * Vector3(0, 1, 0);
    Vector3 zAxis = rot * Vector3(0, 0, 1);
    
    float lineThickness = 4.0f;
    float boxSize = scale * 0.12f;
    
    Vector3 xEnd = position + xAxis * scale;
    Vector3 yEnd = position + yAxis * scale;
    Vector3 zEnd = position + zAxis * scale;
    
    // Draw lines to boxes
    Vector3 xLineEnd = xEnd - xAxis * boxSize;
    Vector3 yLineEnd = yEnd - yAxis * boxSize;
    Vector3 zLineEnd = zEnd - zAxis * boxSize;
    
    // X axis (Red)
    Vector3 screenStart = WorldToScreen(position, viewMatrix, projMatrix, viewportPos, viewportSize);
    Vector3 screenEnd = WorldToScreen(xLineEnd, viewMatrix, projMatrix, viewportPos, viewportSize);
    if (screenStart.z >= 0 && screenStart.z <= 1 && screenEnd.z >= 0 && screenEnd.z <= 1) {
        ImU32 color = GetAxisColor(GizmoAxis::X, m_highlightedAxis == GizmoAxis::X);
        drawList->AddLine(ImVec2(screenStart.x, screenStart.y), ImVec2(screenEnd.x, screenEnd.y), color, lineThickness);
    }
    DrawBox(drawList, xEnd, Vector3(boxSize, boxSize, boxSize), rot,
            GetAxisColor(GizmoAxis::X, m_highlightedAxis == GizmoAxis::X), 2.0f,
            viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Y axis (Green)
    screenEnd = WorldToScreen(yLineEnd, viewMatrix, projMatrix, viewportPos, viewportSize);
    if (screenStart.z >= 0 && screenStart.z <= 1 && screenEnd.z >= 0 && screenEnd.z <= 1) {
        ImU32 color = GetAxisColor(GizmoAxis::Y, m_highlightedAxis == GizmoAxis::Y);
        drawList->AddLine(ImVec2(screenStart.x, screenStart.y), ImVec2(screenEnd.x, screenEnd.y), color, lineThickness);
    }
    DrawBox(drawList, yEnd, Vector3(boxSize, boxSize, boxSize), rot,
            GetAxisColor(GizmoAxis::Y, m_highlightedAxis == GizmoAxis::Y), 2.0f,
            viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Z axis (Blue)
    screenEnd = WorldToScreen(zLineEnd, viewMatrix, projMatrix, viewportPos, viewportSize);
    if (screenStart.z >= 0 && screenStart.z <= 1 && screenEnd.z >= 0 && screenEnd.z <= 1) {
        ImU32 color = GetAxisColor(GizmoAxis::Z, m_highlightedAxis == GizmoAxis::Z);
        drawList->AddLine(ImVec2(screenStart.x, screenStart.y), ImVec2(screenEnd.x, screenEnd.y), color, lineThickness);
    }
    DrawBox(drawList, zEnd, Vector3(boxSize, boxSize, boxSize), rot,
            GetAxisColor(GizmoAxis::Z, m_highlightedAxis == GizmoAxis::Z), 2.0f,
            viewMatrix, projMatrix, viewportPos, viewportSize);
    
    // Center cube for uniform scale
    ImU32 centerColor = (m_highlightedAxis == GizmoAxis::All) ? m_colorHighlight : IM_COL32(180, 180, 180, 255);
    DrawBox(drawList, position, Vector3(boxSize * 1.5f, boxSize * 1.5f, boxSize * 1.5f), rot,
            centerColor, 2.0f,
            viewMatrix, projMatrix, viewportPos, viewportSize);
}

} // namespace BGE