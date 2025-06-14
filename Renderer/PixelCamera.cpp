#include "PixelCamera.h"
#include "../Core/Logger.h" // For logging

namespace BGE {

PixelCamera::PixelCamera() : m_position(0.0f, 0.0f), m_zoom(1), m_screenWidth(1.0f), m_screenHeight(1.0f) {
    BGE_LOG_INFO("PixelCamera", "PixelCamera created with default position (0,0) and zoom 1.");
    // Initialize with a default screen size until SetProjection is called
    // This default might be based on a common resolution or a config value
    SetProjection(1280, 720); // Defaulting to a common 720p resolution
}

void PixelCamera::SetPosition(const Vector2& pos) {
    m_position.x = std::floorf(pos.x);
    m_position.y = std::floorf(pos.y);
    // BGE_LOG_TRACE("PixelCamera", "Position set to (" + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ")");
}

Vector2 PixelCamera::GetPosition() const {
    return m_position;
}

void PixelCamera::SetZoom(int zoom) {
    if (zoom < 1) {
        m_zoom = 1;
        BGE_LOG_WARNING("PixelCamera", "Zoom level cannot be less than 1. Clamped to 1.");
    } else {
        m_zoom = zoom;
    }
    // BGE_LOG_TRACE("PixelCamera", "Zoom set to " + std::to_string(m_zoom));
}

int PixelCamera::GetZoom() const {
    return m_zoom;
}

void PixelCamera::SetProjection(float screenWidth, float screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    // BGE_LOG_INFO("PixelCamera", "Projection set for screen size: "
    //           + std::to_string(screenWidth) + "x" + std::to_string(screenHeight));
}

Matrix4 PixelCamera::GetViewMatrix() const {
    // Calculate the effective screen dimensions based on zoom
    // Zoomming in means showing less of the world, so divide screen dimensions by zoom
    float effectiveScreenWidth = m_screenWidth / m_zoom;
    float effectiveScreenHeight = m_screenHeight / m_zoom;

    // Calculate the orthographic projection bounds
    // The camera's position is the center of the view
    float left = m_position.x - effectiveScreenWidth / 2.0f;
    float right = m_position.x + effectiveScreenWidth / 2.0f;
    float bottom = m_position.y - effectiveScreenHeight / 2.0f; // In many 2D views, positive Y is down
    float top = m_position.y + effectiveScreenHeight / 2.0f;    // So bottom > top if Y is up in world, top > bottom if Y is down.
                                                            // Assuming world Y is upwards for now.
                                                            // If pixel art means pixel coords (0,0 top-left), then bottom/top might be swapped or calculated differently.
                                                            // For pixel-perfect, ensure these are on pixel boundaries.
                                                            // Let's use floorf for consistency, though the position snapping is primary.
    left = std::floorf(left);
    right = std::floorf(right);
    bottom = std::floorf(bottom);
    top = std::floorf(top);

    // BGE::Matrix4::Orthographic expects: left, right, bottom, top, near, far
    // For 2D, near and far can be -1 and 1 or similar.
    Matrix4 view = Matrix4::Orthographic(left, right, bottom, top, -1.0f, 1.0f);

    // The view matrix in typical graphics pipelines is the inverse of the camera's transformation.
    // However, an orthographic matrix constructed this way often directly serves as the combined view-projection.
    // If we needed a separate camera transform (e.g. if Matrix4::Orthographic was just projection),
    // it would be: Matrix4::Translation(Vector3(-m_position.x, -m_position.y, 0)).
    // But since Orthographic takes world-space bounds, it should be correct.

    return view;
}

Vector2 PixelCamera::ScreenToWorld(float screenX, float screenY) const {
    // Calculate the effective screen dimensions based on zoom
    float effectiveScreenWidth = m_screenWidth / m_zoom;
    float effectiveScreenHeight = m_screenHeight / m_zoom;
    
    // Convert screen coordinates (0,0 at top-left) to normalized device coordinates (-1 to 1)
    float ndcX = (2.0f * screenX / m_screenWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / m_screenHeight); // Flip Y for OpenGL coordinates
    
    // Convert from NDC to world coordinates
    float worldX = m_position.x + (ndcX * effectiveScreenWidth * 0.5f);
    float worldY = m_position.y + (ndcY * effectiveScreenHeight * 0.5f);
    
    return Vector2{worldX, worldY};
}

} // namespace BGE
