#pragma once

#include <vector> // For std::vector

// Basic MaterialID enum definition since MaterialID.h is not present
enum class MaterialID {
    Empty,
    Sand, // Example material
    Water // Example material
    // Add other material types as needed
};

class Grid {
public:
    Grid(int width, int height);
    ~Grid();

    void update(float deltaTime);
    void placeSand(int x, int y); // This might change to use MaterialID

    // Getter methods
    int getWidth() const;
    int getHeight() const;
    MaterialID getMaterialID(int x, int y) const;

    // Add other grid-related methods here

private:
    int m_width;
    int m_height;
    // Assuming m_grid_data is a 2D vector of MaterialID
    std::vector<std::vector<MaterialID>> m_grid_data;
};
