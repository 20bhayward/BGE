#pragma once

#include "../Core/Math/Vector2.h"
#include "../Core/Math/Matrix4.h"
#include <cmath> // For floorf

namespace BGE {

class PixelCamera {
public:
    PixelCamera();

    void SetPosition(const Vector2& pos);
    Vector2 GetPosition() const;

    void SetZoom(int zoom);
    int GetZoom() const;

    // Gets the view matrix for rendering
    Matrix4 GetViewMatrix() const;

    // Optional: If the projection needs to be configured (e.g., based on screen size)
    void SetProjection(float screenWidth, float screenHeight);

private:
    Vector2 m_position;
    int m_zoom;
    float m_screenWidth;
    float m_screenHeight;
};

} // namespace BGE
