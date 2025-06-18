#pragma once

#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"
#include <imgui.h>
#include <functional>

namespace BGE {

enum class Gizmo2DMode {
    Translate,
    Rotate,
    Scale
};

enum class Gizmo2DAxis {
    None = 0,
    X = 1,
    Y = 2,
    Both = 3
};

class Gizmo2D {
public:
    Gizmo2D();
    
    void SetMode(Gizmo2DMode mode) { m_mode = mode; }
    Gizmo2DMode GetMode() const { return m_mode; }
    
    void SetPosition(const Vector2& pos) { m_position = pos; }
    Vector2 GetPosition() const { return m_position; }
    
    void SetRotation(float rotation) { m_rotation = rotation; }
    float GetRotation() const { return m_rotation; }
    
    void SetScale(const Vector2& scale) { m_scale = scale; }
    Vector2 GetScale() const { return m_scale; }
    
    // Render the gizmo
    void Render(ImDrawList* drawList, const Vector2& screenPos, float zoom);
    
    // Set the world-to-screen transformation function
    using WorldToScreenFunc = std::function<Vector2(const Vector2&)>;
    void SetWorldToScreenFunc(WorldToScreenFunc func) { m_worldToScreenFunc = func; }
    
    // Set the screen-to-world transformation function
    using ScreenToWorldFunc = std::function<Vector2(const Vector2&)>;
    void SetScreenToWorldFunc(ScreenToWorldFunc func) { m_screenToWorldFunc = func; }
    
    // Handle input - returns true if gizmo was interacted with
    bool HandleInput(const Vector2& mousePos, bool mouseDown, bool mouseDragging, float zoom = 1.0f);
    
    // Get current active axis
    Gizmo2DAxis GetActiveAxis() const { return m_activeAxis; }
    
    // Get current hovered axis
    Gizmo2DAxis GetHoveredAxis() const { return m_hoveredAxis; }
    
    // Check if mouse position is over gizmo (for early hover detection)
    bool IsMouseOverGizmo(const Vector2& mousePos, float zoom = 1.0f);
    
    // Callbacks
    using TransformCallback = std::function<void(const Vector2& pos, float rotation, const Vector2& scale)>;
    void SetCallback(TransformCallback callback) { m_callback = callback; }
    
private:
    void RenderTranslateGizmo(ImDrawList* drawList, const Vector2& screenPos, float size);
    void RenderRotateGizmo(ImDrawList* drawList, const Vector2& screenPos, float size);
    void RenderScaleGizmo(ImDrawList* drawList, const Vector2& screenPos, float size);
    
    Gizmo2DAxis GetAxisUnderMouse(const Vector2& mousePos, const Vector2& screenPos, float size, float tolerance);
    
    Gizmo2DMode m_mode = Gizmo2DMode::Translate;
    Vector2 m_position{0, 0};
    float m_rotation = 0.0f;
    Vector2 m_scale{1, 1};
    
    // Interaction state
    Gizmo2DAxis m_activeAxis = Gizmo2DAxis::None;
    Gizmo2DAxis m_hoveredAxis = Gizmo2DAxis::None;
    Vector2 m_dragStart;
    Vector2 m_initialPosition;
    float m_initialRotation;
    Vector2 m_initialScale;
    
    TransformCallback m_callback;
    WorldToScreenFunc m_worldToScreenFunc;
    ScreenToWorldFunc m_screenToWorldFunc;
    
    // Helper functions
    Vector2 WorldToScreen(const Vector2& worldPos);
    Vector2 ScreenToWorld(const Vector2& screenPos);
    
    // Colors
    ImU32 m_colorX = IM_COL32(221, 56, 53, 255);      // Red
    ImU32 m_colorY = IM_COL32(130, 214, 29, 255);     // Green
    ImU32 m_colorBoth = IM_COL32(255, 235, 4, 255);   // Yellow
    ImU32 m_colorHighlight = IM_COL32(255, 255, 255, 255); // White
    ImU32 m_colorRotation = IM_COL32(50, 133, 253, 255); // Blue
};

} // namespace BGE