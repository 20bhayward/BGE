#pragma once

// Forward declaration of Grid to avoid circular dependency
class Grid;

class Renderer {
public:
    Renderer(unsigned int width, unsigned int height);
    ~Renderer();

    void render(const Grid& grid);
    // Add other rendering-related methods here

private:
    unsigned int m_width;
    unsigned int m_height;
    // Placeholder for rendering resources
    void* m_render_target;
};
