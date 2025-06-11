#pragma once

#include "MaterialRegistry.h" // Added
#include <vector>             // Added

class Grid {
public:
    Grid(int width, int height, MaterialRegistry& materialRegistry); // Updated constructor
    ~Grid();

    void update(float deltaTime); // Changed signature
    MaterialID getCell(int x, int y) const; // Added
    void setCell(int x, int y, MaterialID material); // Added
    void swapBuffers(); // Added
    // void placeSand(int x, int y); // Removed

    // Add other grid-related methods here

private:
    size_t toIndex(int x, int y) const; // Added private helper

    int m_width;
    int m_height;

    // Changed m_grid_data to specific cell buffers and material registry
    std::vector<MaterialID> m_readCells;
    std::vector<MaterialID> m_writeCells;
    MaterialRegistry& m_materialRegistry;
};
