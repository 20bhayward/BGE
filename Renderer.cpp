#include "Renderer.h"
#include "Grid.h" // For the render method parameter
#include <iostream> // For debug messages

Renderer::Renderer(unsigned int width, unsigned int height)
    : m_width(width), m_height(height), m_render_target(nullptr) {
    std::cout << "Renderer: Constructor called with width=" << width << ", height=" << height << std::endl;
    // Initialize rendering resources here (e.g., shaders, framebuffers).
    // m_render_target is a placeholder for now.
}

Renderer::~Renderer() {
    std::cout << "Renderer: Destructor called." << std::endl;
    // Clean up rendering resources.
}

void Renderer::render(const Grid& grid) {
    // std::cout << "Renderer: render called." << std::endl; // Can be too verbose
    // Actual rendering logic will go here.
    // This method would iterate over the grid data and draw it to the screen.
    // For example, using OpenGL, SFML, or another graphics library.
}
