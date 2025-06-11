#pragma once

#include <SFML/Graphics.hpp> // Added this line
#include "Window.h"          // Added this line
#include "Grid.h"            // Added this line
#include "MaterialRegistry.h" // Added this line


class Renderer {
public:
    Renderer(const MaterialRegistry& materialRegistry); // Changed constructor
    ~Renderer();

    void render(const Grid& grid, Window& window); // Changed signature
    // Add other rendering-related methods here

private:
    sf::VertexArray m_vertices; // Replaced m_render_target
    const MaterialRegistry& m_materialRegistry; // Added member
};
