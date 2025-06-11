#pragma once

class Grid {
public:
    Grid(int width, int height);
    ~Grid();

    void update(float deltaTime);
    void placeSand(int x, int y);
    // Add other grid-related methods here

private:
    int m_width;
    int m_height;
    // Placeholder for grid data structure
    void* m_grid_data;
};
