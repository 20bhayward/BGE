#include "Grid.h"
#include <iostream> // For debug messages

Grid::Grid(int width, int height) : m_width(width), m_height(height), m_grid_data(nullptr) {
    std::cout << "Grid: Constructor called with width=" << width << ", height=" << height << std::endl;
    // Initialize m_grid_data here if it's a pointer to dynamically allocated memory
    // For example, m_grid_data = new Cell[width * height];
    // For now, it's just a void pointer placeholder.
}

Grid::~Grid() {
    std::cout << "Grid: Destructor called." << std::endl;
    // If m_grid_data was dynamically allocated, delete it here.
    // For example, if m_grid_data was Cell*, then delete[] static_cast<Cell*>(m_grid_data);
}

void Grid::update(float deltaTime) {
    // std::cout << "Grid: update called with deltaTime = " << deltaTime << std::endl; // Can be too verbose
    // Actual grid simulation logic (sand falling, etc.) will go here.
}

void Grid::placeSand(int x, int y) {
    std::cout << "Grid: placeSand called at (" << x << ", " << y << ")" << std::endl;
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        // Logic to place sand at the given grid cell.
        // This would involve modifying m_grid_data.
        std::cout << "Grid: Sand placed at valid coordinates (" << x << ", " << y << ")" << std::endl;
    } else {
        std::cout << "Grid: Attempted to place sand outside grid boundaries at (" << x << ", " << y << ")" << std::endl;
    }
}
