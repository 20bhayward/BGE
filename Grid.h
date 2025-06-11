#pragma once

#include <vector> // For std::vector

// Basic MaterialID enum definition since MaterialID.h is not present
enum class MaterialID {
    Empty,
    Sand, // Example material
    Water // Example material
    // Add other material types as needed
};
=======
#include "MaterialRegistry.h" // Added
#include <vector>             // Added

class Grid {
public:
    Grid(int width, int height, MaterialRegistry& materialRegistry); // Updated constructor
    ~Grid();
    void update(float deltaTime);
    void placeSand(int x, int y); // This might change to use MaterialID

    // Getter methods
    int getWidth() const;
    int getHeight() const;
    MaterialID getMaterialID(int x, int y) const;
    MaterialID getCell(int x, int y) const; // Added
    void setCell(int x, int y, MaterialID material); // Added
    void swapBuffers(); // Added
    // Add other grid-related methods here

private:
    size_t toIndex(int x, int y) const; // Added private helper

    int m_width;
    int m_height;
    // Assuming m_grid_data is a 2D vector of MaterialID
    std::vector<std::vector<MaterialID>> m_grid_data;

    // Changed m_grid_data to specific cell buffers and material registry
    std::vector<MaterialID> m_readCells;
    std::vector<MaterialID> m_writeCells;
    MaterialRegistry& m_materialRegistry;
};
