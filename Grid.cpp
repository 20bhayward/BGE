#include "Grid.h"
#include <vector>
#include <algorithm> // For std::fill and std::swap
#include <cstdlib>   // For rand()
// Removed iostream as it's no longer used for debug messages here.

Grid::Grid(int width, int height, MaterialRegistry& materialRegistry)
    : m_width(width), m_height(height), m_materialRegistry(materialRegistry) {
    m_readCells.resize(static_cast<size_t>(width) * height);
    m_writeCells.resize(static_cast<size_t>(width) * height);

    std::fill(m_readCells.begin(), m_readCells.end(), MaterialID::Empty);
    std::fill(m_writeCells.begin(), m_writeCells.end(), MaterialID::Empty);
}

Grid::~Grid() {
    // No explicit cleanup needed for std::vector members like m_readCells and m_writeCells.
    // m_materialRegistry is a reference, so its lifetime is managed externally.
}

size_t Grid::toIndex(int x, int y) const {
    // Assuming x and y are always within bounds [0, m_width-1] and [0, m_height-1] respectively
    // when this function is called internally by getCell/setCell after bounds checking.
    return static_cast<size_t>(y) * m_width + static_cast<size_t>(x);
}

void Grid::swapBuffers() {
    m_readCells.swap(m_writeCells);
}

MaterialID Grid::getCell(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        // Return a "boundary" material or handle error. Rock is a solid, unmovable material.
        return MaterialID::Rock;
    }
    return m_readCells[toIndex(x, y)];
}

void Grid::setCell(int x, int y, MaterialID material) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        // Out of bounds, do nothing or log an error.
        return;
    }
    m_writeCells[toIndex(x, y)] = material;
}

void Grid::update(float deltaTime) { // Added deltaTime parameter
    // deltaTime is currently unused in the simulation logic.
    m_writeCells = m_readCells;

    for (int y = m_height - 2; y >= 0; --y) {
        for (int x = 0; x < m_width; ++x) {
            MaterialID currentMaterialID = m_readCells[toIndex(x, y)];

            if (currentMaterialID == MaterialID::Empty || currentMaterialID == MaterialID::Rock) {
                continue;
            }
            const MaterialDefinition& currentMaterialDef = m_materialRegistry.getMaterial(currentMaterialID);

            switch (currentMaterialID) {
                case MaterialID::Sand: {
                    MaterialID belowMaterialID = m_readCells[toIndex(x, y + 1)];
                    const MaterialDefinition& belowMaterialDef = m_materialRegistry.getMaterial(belowMaterialID);
                    if (belowMaterialDef.density < currentMaterialDef.density) {
                        m_writeCells[toIndex(x, y)] = belowMaterialID;
                        m_writeCells[toIndex(x, y + 1)] = currentMaterialID;
                        continue;
                    }

                    bool checkLeftFirst = (rand() % 2 == 0);
                    for (int i = 0; i < 2; ++i) {
                        int diagX = x + (checkLeftFirst ? -1 : 1);
                        if (i == 1) diagX = x + (checkLeftFirst ? 1 : -1);
                        if (diagX >= 0 && diagX < m_width) {
                            MaterialID diagMaterialID = m_readCells[toIndex(diagX, y + 1)];
                            const MaterialDefinition& diagMaterialDef = m_materialRegistry.getMaterial(diagMaterialID);
                            if (diagMaterialDef.density < currentMaterialDef.density) {
                                m_writeCells[toIndex(x, y)] = MaterialID::Empty;
                                m_writeCells[toIndex(diagX, y + 1)] = currentMaterialID;
                                goto next_particle_in_grid;
                            }
                        }
                    }
                    break;
                }
                case MaterialID::Water: {
                    MaterialID belowWaterMaterialID = m_readCells[toIndex(x, y + 1)];
                    const MaterialDefinition& belowWaterMaterialDef = m_materialRegistry.getMaterial(belowWaterMaterialID);
                    if (belowWaterMaterialDef.density < currentMaterialDef.density) {
                        m_writeCells[toIndex(x, y)] = belowWaterMaterialID;
                        m_writeCells[toIndex(x, y + 1)] = currentMaterialID;
                        continue;
                    }

                    int dispersionLimit = 10;
                    int furthestRightX = -1; // Furthest it can go to the right
                    for (int i = 1; i <= dispersionLimit; ++i) {
                        int targetX = x + i;
                        if (targetX >= m_width) break;
                        MaterialID targetMaterialID = m_readCells[toIndex(targetX, y)];
                        const MaterialDefinition& targetMaterialDef = m_materialRegistry.getMaterial(targetMaterialID);
                        if (targetMaterialDef.density < currentMaterialDef.density) { // Can flow into less dense (includes Empty)
                            furthestRightX = targetX;
                        } else { break; }
                    }

                    int furthestLeftX = -1; // Furthest it can go to the left
                    for (int i = 1; i <= dispersionLimit; ++i) {
                        int targetX = x - i;
                        if (targetX < 0) break;
                        MaterialID targetMaterialID = m_readCells[toIndex(targetX, y)];
                        const MaterialDefinition& targetMaterialDef = m_materialRegistry.getMaterial(targetMaterialID);
                        if (targetMaterialDef.density < currentMaterialDef.density) { // Can flow into less dense
                            furthestLeftX = targetX;
                        } else { break; }
                    }

                    int finalTargetX = -1;
                    bool canGoLeft = (furthestLeftX != -1);
                    bool canGoRight = (furthestRightX != -1);

                    if (canGoLeft && canGoRight) {
                        finalTargetX = (rand() % 2 == 0) ? furthestLeftX : furthestRightX;
                    } else if (canGoLeft) {
                        finalTargetX = furthestLeftX;
                    } else if (canGoRight) {
                        finalTargetX = furthestRightX;
                    }

                    if (finalTargetX != -1) {
                        // Water moves to finalTargetX, original cell (x,y) becomes what was at finalTargetX
                        // If finalTargetX was Empty, (x,y) becomes Empty.
                        // This is a swap to allow less dense materials (like air/empty) to be displaced.
                        MaterialID materialAtTarget = m_readCells[toIndex(finalTargetX, y)];
                        m_writeCells[toIndex(x, y)] = materialAtTarget;
                        m_writeCells[toIndex(finalTargetX, y)] = currentMaterialID;
                        continue;
                    }
                    break;
                }
                default:
                    break;
            }
            next_particle_in_grid:;
        }
    }
}

// Removed Grid::placeSand(int x, y)
