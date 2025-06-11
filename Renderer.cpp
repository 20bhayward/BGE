#include "Renderer.h"
#include "Grid.h"
#include "MaterialRegistry.h"
#include "Window.h" // Required for Window::draw

// Constructor
Renderer::Renderer(const MaterialRegistry& materialRegistry)
    : m_materialRegistry(materialRegistry) {
    m_vertices.setPrimitiveType(sf::Points);
    // m_materialRegistry is initialized in the member initializer list
}

// Destructor - SFML resources like sf::VertexArray are managed automatically
Renderer::~Renderer() {
    // No explicit cleanup needed for m_vertices
}

// Render the grid to the window
void Renderer::render(const Grid& grid, Window& window) {
    m_vertices.clear(); // Clear vertices from the previous frame

    unsigned int gridWidth = grid.getWidth();
    unsigned int gridHeight = grid.getHeight();

    for (unsigned int y = 0; y < gridHeight; ++y) {
        for (unsigned int x = 0; x < gridWidth; ++x) {
            MaterialID materialID = grid.getMaterialID(x, y);

            // Only draw if the cell is not empty
            if (materialID != MaterialID::Empty) { // Assuming MaterialID::Empty is defined
                sf::Color color = m_materialRegistry.getColor(materialID);
                m_vertices.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), color));
            }
        }
    }

    window.draw(m_vertices); // Draw all collected vertices
}
