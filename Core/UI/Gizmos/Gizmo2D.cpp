#include "Gizmo2D.h"
#include <cmath>
#include <functional>
#include <cstdio>

namespace BGE {

Gizmo2D::Gizmo2D() {
}

void Gizmo2D::Render(ImDrawList* drawList, const Vector2& screenPos, float /*zoom*/) {
    float size = 60.0f; // Fixed screen size regardless of zoom
    
    // Render debug output (reduced)
    static int renderCounter = 0;
    if (++renderCounter % 120 == 0) {
        printf("Gizmo2D::Render at screen pos: (%.1f, %.1f), mode: %d\n", 
               screenPos.x, screenPos.y, (int)m_mode);
    }
    
    switch (m_mode) {
        case Gizmo2DMode::Translate:
            RenderTranslateGizmo(drawList, screenPos, size);
            break;
        case Gizmo2DMode::Rotate:
            RenderRotateGizmo(drawList, screenPos, size);
            break;
        case Gizmo2DMode::Scale:
            RenderScaleGizmo(drawList, screenPos, size);
            break;
    }
}

void Gizmo2D::RenderTranslateGizmo(ImDrawList* drawList, const Vector2& screenPos, float size) {
    ImVec2 center(screenPos.x, screenPos.y);
    
    // DEBUG: Draw click areas (showing actual hit detection zones)
    float centerSquareSize = size * 0.15f; // Match the visual and hit detection size
    float halfCenterSquare = centerSquareSize * 0.5f;
    
    // Center square click area (actual hit detection area)
    ImVec2 centerClickMin(center.x - halfCenterSquare - 5, center.y - halfCenterSquare - 5);
    ImVec2 centerClickMax(center.x + halfCenterSquare + 5, center.y + halfCenterSquare + 5);
    drawList->AddRect(centerClickMin, centerClickMax, IM_COL32(255, 255, 0, 150), 0.0f, 0, 1.0f); // Yellow outline
    
    // X axis click area (actual hit detection area) - fixed tolerance
    float debugTolerance = 10.0f; // Fixed tolerance regardless of zoom
    ImVec2 xClickMin(center.x, center.y - debugTolerance);
    ImVec2 xClickMax(center.x + size + debugTolerance * 0.5f, center.y + debugTolerance);
    drawList->AddRect(xClickMin, xClickMax, IM_COL32(255, 0, 0, 150), 0.0f, 0, 1.0f); // Red outline
    
    // Y axis click area (actual hit detection area) - use zoom-scaled tolerance with reduced extension
    ImVec2 yClickMin(center.x - debugTolerance, center.y - size - debugTolerance * 0.5f);
    ImVec2 yClickMax(center.x + debugTolerance, center.y);
    drawList->AddRect(yClickMin, yClickMax, IM_COL32(0, 255, 0, 150), 0.0f, 0, 1.0f); // Green outline
    
    // Draw center square first (behind arrows) - positioned to fit inside arrow intersection
    ImU32 bothColor = (m_hoveredAxis == Gizmo2DAxis::Both || m_activeAxis == Gizmo2DAxis::Both) ? 
                      m_colorHighlight : IM_COL32(50, 133, 253, 80); // Transparent blue
    ImVec2 squareMin(center.x - halfCenterSquare, center.y - halfCenterSquare);
    ImVec2 squareMax(center.x + halfCenterSquare, center.y + halfCenterSquare);
    drawList->AddRectFilled(squareMin, squareMax, bothColor);
    if (m_hoveredAxis == Gizmo2DAxis::Both || m_activeAxis == Gizmo2DAxis::Both) {
        drawList->AddRect(squareMin, squareMax, m_colorHighlight, 0.0f, 0, 2.0f);
    }
    
    // Draw X axis (horizontal)
    ImU32 xColor = (m_hoveredAxis == Gizmo2DAxis::X || m_activeAxis == Gizmo2DAxis::X) ? m_colorHighlight : m_colorX;
    ImVec2 xStart = center;
    ImVec2 xEnd(center.x + size, center.y);
    drawList->AddLine(xStart, xEnd, xColor, 3.0f);
    
    // X axis arrow (cone-like)
    float arrowSize = size * 0.2f;
    ImVec2 xArrow1(xEnd.x - arrowSize, xEnd.y - arrowSize * 0.5f);
    ImVec2 xArrow2(xEnd.x - arrowSize, xEnd.y + arrowSize * 0.5f);
    ImVec2 xArrowPoints[3] = { xEnd, xArrow1, xArrow2 };
    drawList->AddConvexPolyFilled(xArrowPoints, 3, xColor);
    
    // Draw Y axis (vertical - pointing up in screen space)
    ImU32 yColor = (m_hoveredAxis == Gizmo2DAxis::Y || m_activeAxis == Gizmo2DAxis::Y) ? m_colorHighlight : m_colorY;
    ImVec2 yStart = center;
    ImVec2 yEnd(center.x, center.y - size); // Negative Y for up
    drawList->AddLine(yStart, yEnd, yColor, 3.0f);
    
    // Y axis arrow (cone-like)
    ImVec2 yArrow1(yEnd.x - arrowSize * 0.5f, yEnd.y + arrowSize);
    ImVec2 yArrow2(yEnd.x + arrowSize * 0.5f, yEnd.y + arrowSize);
    ImVec2 yArrowPoints[3] = { yEnd, yArrow1, yArrow2 };
    drawList->AddConvexPolyFilled(yArrowPoints, 3, yColor);
}

void Gizmo2D::RenderRotateGizmo(ImDrawList* drawList, const Vector2& screenPos, float size) {
    ImVec2 center(screenPos.x, screenPos.y);
    
    // Draw rotation circle
    ImU32 color = (m_hoveredAxis != Gizmo2DAxis::None || m_activeAxis != Gizmo2DAxis::None) ? m_colorHighlight : m_colorRotation;
    drawList->AddCircle(center, size, color, 48, 3.0f);
    
    // Draw rotation handle
    float handleAngle = -m_rotation; // Negative because screen Y is flipped
    float handleX = center.x + cosf(handleAngle) * size;
    float handleY = center.y + sinf(handleAngle) * size;
    drawList->AddCircleFilled(ImVec2(handleX, handleY), 8.0f, color);
    
    // Draw rotation angle text when rotating
    if (m_activeAxis != Gizmo2DAxis::None) {
        float degrees = m_rotation * 180.0f / 3.14159265f;
        char angleText[32];
        sprintf(angleText, "%.1f°", degrees);
        ImVec2 textPos(center.x + 10, center.y - size - 20);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), angleText);
    }
}

void Gizmo2D::RenderScaleGizmo(ImDrawList* drawList, const Vector2& screenPos, float size) {
    ImVec2 center(screenPos.x, screenPos.y);
    
    // DEBUG: Draw click areas for scale - fixed tolerance
    float debugTolerance = 10.0f; // Fixed tolerance regardless of zoom
    // Center box click area
    ImVec2 centerClickMin(center.x - debugTolerance, center.y - debugTolerance);
    ImVec2 centerClickMax(center.x + debugTolerance, center.y + debugTolerance);
    drawList->AddRect(centerClickMin, centerClickMax, IM_COL32(255, 255, 0, 150), 0.0f, 0, 1.0f); // Yellow outline
    
    // X handle click area (reduced extension)
    ImVec2 xClickMin(center.x, center.y - debugTolerance);
    ImVec2 xClickMax(center.x + size + debugTolerance * 0.5f, center.y + debugTolerance);
    drawList->AddRect(xClickMin, xClickMax, IM_COL32(255, 0, 0, 150), 0.0f, 0, 1.0f); // Red outline
    
    // Y handle click area (reduced extension)
    ImVec2 yClickMin(center.x - debugTolerance, center.y - size - debugTolerance * 0.5f);
    ImVec2 yClickMax(center.x + debugTolerance, center.y);
    drawList->AddRect(yClickMin, yClickMax, IM_COL32(0, 255, 0, 150), 0.0f, 0, 1.0f); // Green outline
    
    // Draw X axis line
    ImU32 xColor = (m_hoveredAxis == Gizmo2DAxis::X || m_activeAxis == Gizmo2DAxis::X) ? m_colorHighlight : m_colorX;
    ImVec2 xEnd(center.x + size, center.y);
    drawList->AddLine(center, xEnd, xColor, 3.0f);
    
    // X axis handle
    ImVec2 xBoxMin(xEnd.x - 8, xEnd.y - 8);
    ImVec2 xBoxMax(xEnd.x + 8, xEnd.y + 8);
    drawList->AddRectFilled(xBoxMin, xBoxMax, xColor);
    drawList->AddRect(xBoxMin, xBoxMax, IM_COL32(0, 0, 0, 100), 0.0f, 0, 1.0f);
    
    // Draw Y axis line
    ImU32 yColor = (m_hoveredAxis == Gizmo2DAxis::Y || m_activeAxis == Gizmo2DAxis::Y) ? m_colorHighlight : m_colorY;
    ImVec2 yEnd(center.x, center.y - size);
    drawList->AddLine(center, yEnd, yColor, 3.0f);
    
    // Y axis handle
    ImVec2 yBoxMin(yEnd.x - 8, yEnd.y - 8);
    ImVec2 yBoxMax(yEnd.x + 8, yEnd.y + 8);
    drawList->AddRectFilled(yBoxMin, yBoxMax, yColor);
    drawList->AddRect(yBoxMin, yBoxMax, IM_COL32(0, 0, 0, 100), 0.0f, 0, 1.0f);
    
    // Draw center box for uniform scale
    ImU32 bothColor = (m_hoveredAxis == Gizmo2DAxis::Both || m_activeAxis == Gizmo2DAxis::Both) ? m_colorHighlight : m_colorBoth;
    ImVec2 centerBoxMin(center.x - 8, center.y - 8);
    ImVec2 centerBoxMax(center.x + 8, center.y + 8);
    drawList->AddRectFilled(centerBoxMin, centerBoxMax, bothColor);
    drawList->AddRect(centerBoxMin, centerBoxMax, IM_COL32(0, 0, 0, 100), 0.0f, 0, 1.0f);
}

bool Gizmo2D::HandleInput(const Vector2& mousePos, bool mouseDown, bool mouseDragging, float /*zoom*/) {
    Vector2 screenPos = WorldToScreen(m_position);
    float gizmoSize = 60.0f; // Fixed screen size regardless of zoom
    float tolerance = 10.0f; // Fixed tolerance regardless of zoom
    
    // Debug output
    static int debugCounter = 0;
    if (++debugCounter % 60 == 0) { // Print once per second at 60fps
        printf("Gizmo2D::HandleInput - mousePos: (%.1f, %.1f), screenPos: (%.1f, %.1f), mouseDown: %d, mouseDragging: %d\n", 
               mousePos.x, mousePos.y, screenPos.x, screenPos.y, mouseDown, mouseDragging);
    }
    
    if (!mouseDragging && m_activeAxis == Gizmo2DAxis::None) {
        // Update hover state
        m_hoveredAxis = GetAxisUnderMouse(mousePos, screenPos, gizmoSize, tolerance);
        if (m_hoveredAxis != Gizmo2DAxis::None && debugCounter % 60 == 0) {
            printf("Hovering over axis: %d\n", (int)m_hoveredAxis);
        }
    }
    
    if (mouseDown && m_activeAxis == Gizmo2DAxis::None && m_hoveredAxis != Gizmo2DAxis::None) {
        // Start dragging
        printf("Starting drag on axis: %d\n", (int)m_hoveredAxis);
        m_activeAxis = m_hoveredAxis;
        m_dragStart = mousePos;
        m_initialPosition = m_position;
        m_initialRotation = m_rotation;
        m_initialScale = m_scale;
        return true;
    }
    
    if (mouseDragging && m_activeAxis != Gizmo2DAxis::None) {
        // Update transform based on drag
        // Reduced drag debug output
        static int dragCounter = 0;
        if (++dragCounter % 30 == 0) {
            printf("Processing drag: activeAxis=%d, mouseDragging=%d\n", (int)m_activeAxis, mouseDragging);
        }
        Vector2 delta = mousePos - m_dragStart;
        
        switch (m_mode) {
            case Gizmo2DMode::Translate: {
                // Simple approach: work in screen space then convert to world
                Vector2 initialScreenPos = WorldToScreen(m_initialPosition);
                Vector2 newScreenPos = initialScreenPos + delta;
                
                // Apply axis constraints in screen space
                if (m_activeAxis == Gizmo2DAxis::X) {
                    newScreenPos.y = initialScreenPos.y; // Lock Y
                } else if (m_activeAxis == Gizmo2DAxis::Y) {
                    newScreenPos.x = initialScreenPos.x; // Lock X
                }
                
                // Convert back to world position
                Vector2 newWorldPos = ScreenToWorld(newScreenPos);
                m_position = newWorldPos;
                break;
            }
            
            case Gizmo2DMode::Rotate: {
                Vector2 centerToStart = m_dragStart - screenPos;
                Vector2 centerToCurrent = mousePos - screenPos;
                float angle1 = atan2f(centerToStart.y, centerToStart.x);
                float angle2 = atan2f(centerToCurrent.y, centerToCurrent.x);
                // Reverse the direction to match expected rotation
                m_rotation = m_initialRotation - (angle2 - angle1);
                break;
            }
            
            case Gizmo2DMode::Scale: {
                float scaleDelta = (delta.x + delta.y) * 0.01f;
                if (m_activeAxis == Gizmo2DAxis::X) {
                    m_scale.x = std::max(0.1f, m_initialScale.x + scaleDelta);
                } else if (m_activeAxis == Gizmo2DAxis::Y) {
                    m_scale.y = std::max(0.1f, m_initialScale.y + scaleDelta);
                } else if (m_activeAxis == Gizmo2DAxis::Both) {
                    m_scale.x = std::max(0.1f, m_initialScale.x + scaleDelta);
                    m_scale.y = std::max(0.1f, m_initialScale.y + scaleDelta);
                }
                break;
            }
        }
        
        if (m_callback) {
            if (dragCounter % 30 == 0) {
                printf("Calling callback with position: (%.2f, %.2f)\n", m_position.x, m_position.y);
            }
            m_callback(m_position, m_rotation, m_scale);
        } else {
            printf("No callback set!\n");
        }
        return true;
    }
    
    // Only reset active axis when neither mouse down nor dragging
    if (!mouseDown && !mouseDragging && m_activeAxis != Gizmo2DAxis::None) {
        printf("Resetting active axis\n");
        m_activeAxis = Gizmo2DAxis::None;
    }
    
    return false;
}

Gizmo2DAxis Gizmo2D::GetAxisUnderMouse(const Vector2& mousePos, const Vector2& screenPos, float size, float tolerance) {
    Vector2 relPos = mousePos - screenPos;
    
    // Reduced debug output
    static int hitCounter = 0;
    if (++hitCounter % 30 == 0) {
        printf("GetAxisUnderMouse: mouse(%.1f,%.1f) - screen(%.1f,%.1f) = rel(%.1f,%.1f)\n",
               mousePos.x, mousePos.y, screenPos.x, screenPos.y, relPos.x, relPos.y);
    }
    
    switch (m_mode) {
        case Gizmo2DMode::Translate: {
            float centerSquareSize = size * 0.15f; // Match the visual size
            float halfCenterSquare = centerSquareSize * 0.5f;
            
            // Check center square first (make it bigger for easier clicking)
            if (fabs(relPos.x) <= halfCenterSquare + 5 && fabs(relPos.y) <= halfCenterSquare + 5) {
                printf("Hit center square!\n");
                return Gizmo2DAxis::Both;
            }
            // Check X axis - from center to arrow tip (reduced extension)
            if (relPos.x >= 0 && relPos.x <= size + tolerance * 0.5f && fabs(relPos.y) <= tolerance) {
                printf("Hit X axis!\n");
                return Gizmo2DAxis::X;
            }
            // Check Y axis - from center to arrow tip (reduced extension)
            if (relPos.y <= 0 && relPos.y >= -size - tolerance * 0.5f && fabs(relPos.x) <= tolerance) {
                printf("Hit Y axis!\n");
                return Gizmo2DAxis::Y;
            }
            break;
        }
        
        case Gizmo2DMode::Rotate: {
            float dist = relPos.Length();
            // Check if we're within the circle area (not just the ring)
            if (dist <= size + tolerance && dist >= size - tolerance) {
                printf("Hit rotation circle!\n");
                return Gizmo2DAxis::Both; // Use Both for rotation
            }
            break;
        }
        
        case Gizmo2DMode::Scale: {
            printf("Scale detection: rel(%.1f,%.1f) size=%.1f tolerance=%.1f\n", relPos.x, relPos.y, size, tolerance);
            
            // Check center box (bigger)
            if (fabs(relPos.x) <= tolerance && fabs(relPos.y) <= tolerance) {
                printf("Hit scale center! (|%.1f| <= %.1f && |%.1f| <= %.1f)\n", relPos.x, tolerance, relPos.y, tolerance);
                return Gizmo2DAxis::Both;
            }
            // Check X handle - including the line leading to it (reduced extension)
            if ((relPos.x >= 0 && relPos.x <= size + tolerance * 0.5f) && fabs(relPos.y) <= tolerance) {
                printf("Hit X scale handle! (%.1f >= 0 && %.1f <= %.1f && |%.1f| <= %.1f)\n", 
                       relPos.x, relPos.x, size + tolerance * 0.5f, relPos.y, tolerance);
                return Gizmo2DAxis::X;
            }
            // Check Y handle - including the line leading to it (reduced extension)
            if ((relPos.y <= 0 && relPos.y >= -size - tolerance * 0.5f) && fabs(relPos.x) <= tolerance) {
                printf("Hit Y scale handle! (%.1f <= 0 && %.1f >= %.1f && |%.1f| <= %.1f)\n", 
                       relPos.y, relPos.y, -size - tolerance * 0.5f, relPos.x, tolerance);
                return Gizmo2DAxis::Y;
            }
            break;
        }
    }
    
    return Gizmo2DAxis::None;
}

// Helper function to convert world position to screen position
Vector2 Gizmo2D::WorldToScreen(const Vector2& worldPos) {
    if (m_worldToScreenFunc) {
        return m_worldToScreenFunc(worldPos);
    }
    // Fallback: return world position as screen position
    return worldPos;
}

// Helper function to convert screen position to world position
Vector2 Gizmo2D::ScreenToWorld(const Vector2& screenPos) {
    if (m_screenToWorldFunc) {
        return m_screenToWorldFunc(screenPos);
    }
    // Fallback: return screen position as world position
    return screenPos;
}

bool Gizmo2D::IsMouseOverGizmo(const Vector2& mousePos, float /*zoom*/) {
    Vector2 screenPos = WorldToScreen(m_position);
    float gizmoSize = 60.0f; // Fixed screen size regardless of zoom
    float tolerance = 10.0f; // Fixed tolerance regardless of zoom
    
    Gizmo2DAxis axis = GetAxisUnderMouse(mousePos, screenPos, gizmoSize, tolerance);
    return axis != Gizmo2DAxis::None;
}

} // namespace BGE