#include "MaterialBrush.h"
#include "../../Simulation/SimulationWorld.h"
#include <algorithm>
#include <cmath>

namespace BGE {

MaterialBrush::MaterialBrush() = default;

void MaterialBrush::Paint(SimulationWorld* world, int x, int y) {
    if (!world) return;
    
    switch (m_shape) {
        case BrushShape::Circle:
            ApplyCircleBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Square:
            ApplySquareBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Triangle:
            ApplyTriangleBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Diamond:
            ApplyDiamondBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Line:
            ApplyLineBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Cross:
            ApplyCrossBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Star:
            ApplyStarBrush(world, x, y, m_currentMaterial);
            break;
        case BrushShape::Plus:
            ApplyPlusBrush(world, x, y, m_currentMaterial);
            break;
    }
}

void MaterialBrush::Erase(SimulationWorld* world, int x, int y) {
    if (!world) return;
    
    switch (m_shape) {
        case BrushShape::Circle:
            ApplyCircleBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Square:
            ApplySquareBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Triangle:
            ApplyTriangleBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Diamond:
            ApplyDiamondBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Line:
            ApplyLineBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Cross:
            ApplyCrossBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Star:
            ApplyStarBrush(world, x, y, MATERIAL_EMPTY);
            break;
        case BrushShape::Plus:
            ApplyPlusBrush(world, x, y, MATERIAL_EMPTY);
            break;
    }
}

void MaterialBrush::Sample(SimulationWorld* world, int x, int y) {
    if (!world || !world->IsValidPosition(x, y)) return;
    
    m_currentMaterial = world->GetMaterial(x, y);
}

bool MaterialBrush::IsInBrushArea(int centerX, int centerY, int testX, int testY) const {
    int dx = testX - centerX;
    int dy = testY - centerY;
    int adx = std::abs(dx);
    int ady = std::abs(dy);
    
    switch (m_shape) {
        case BrushShape::Circle:
            return (dx * dx + dy * dy) <= (m_size * m_size);
        case BrushShape::Square:
            return adx <= m_size && ady <= m_size;
        case BrushShape::Triangle:
            return ady <= m_size && adx <= (m_size - ady);
        case BrushShape::Diamond:
            return (adx + ady) <= m_size;
        case BrushShape::Line:
            // Diagonal line
            return std::abs(dx - dy) <= 1 && adx <= m_size && ady <= m_size;
        case BrushShape::Cross:
            return (adx <= m_size && ady <= 1) || (ady <= m_size && adx <= 1);
        case BrushShape::Star:
            // 8-spoke star pattern
            return (adx <= m_size || ady <= m_size || (adx + ady) <= m_size * 1.4f);
        case BrushShape::Plus:
            return (adx <= 2 && ady <= m_size) || (ady <= 2 && adx <= m_size);
    }
    return false;
}

void MaterialBrush::ApplyCircleBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    for (int dy = -m_size; dy <= m_size; ++dy) {
        for (int dx = -m_size; dx <= m_size; ++dx) {
            if (dx * dx + dy * dy <= m_size * m_size) {
                int x = centerX + dx;
                int y = centerY + dy;
                if (world->IsValidPosition(x, y)) {
                    world->SetMaterial(x, y, material);
                }
            }
        }
    }
}

void MaterialBrush::ApplySquareBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    for (int dy = -m_size; dy <= m_size; ++dy) {
        for (int dx = -m_size; dx <= m_size; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (world->IsValidPosition(x, y)) {
                world->SetMaterial(x, y, material);
            }
        }
    }
}

void MaterialBrush::ApplyTriangleBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    for (int dy = -m_size; dy <= m_size; ++dy) {
        for (int dx = -m_size; dx <= m_size; ++dx) {
            // Triangle shape: width decreases as we move away from center Y
            int maxWidth = m_size - std::abs(dy);
            if (std::abs(dx) <= maxWidth) {
                int x = centerX + dx;
                int y = centerY + dy;
                if (world->IsValidPosition(x, y)) {
                    world->SetMaterial(x, y, material);
                }
            }
        }
    }
}

void MaterialBrush::ApplyDiamondBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    for (int dy = -m_size; dy <= m_size; ++dy) {
        for (int dx = -m_size; dx <= m_size; ++dx) {
            // Diamond shape: Manhattan distance
            if (std::abs(dx) + std::abs(dy) <= m_size) {
                int x = centerX + dx;
                int y = centerY + dy;
                if (world->IsValidPosition(x, y)) {
                    world->SetMaterial(x, y, material);
                }
            }
        }
    }
}

void MaterialBrush::ApplyLineBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    // Diagonal line from top-left to bottom-right
    for (int i = -m_size; i <= m_size; ++i) {
        int x = centerX + i;
        int y = centerY + i;
        if (world->IsValidPosition(x, y)) {
            world->SetMaterial(x, y, material);
        }
        // Make it slightly thicker
        if (world->IsValidPosition(x + 1, y)) {
            world->SetMaterial(x + 1, y, material);
        }
        if (world->IsValidPosition(x, y + 1)) {
            world->SetMaterial(x, y + 1, material);
        }
    }
}

void MaterialBrush::ApplyCrossBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    // Horizontal line
    for (int dx = -m_size; dx <= m_size; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (world->IsValidPosition(x, y)) {
                world->SetMaterial(x, y, material);
            }
        }
    }
    // Vertical line
    for (int dy = -m_size; dy <= m_size; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (world->IsValidPosition(x, y)) {
                world->SetMaterial(x, y, material);
            }
        }
    }
}

void MaterialBrush::ApplyStarBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    // Create a star with 8 spokes radiating from center
    static const float angles[] = {0.0f, 0.785f, 1.57f, 2.356f, 3.14f, 3.927f, 4.712f, 5.498f}; // 8 directions
    
    for (int i = 0; i < 8; ++i) {
        float angle = angles[i];
        for (int r = 0; r <= m_size; ++r) {
            int x = centerX + static_cast<int>(r * std::cos(angle));
            int y = centerY + static_cast<int>(r * std::sin(angle));
            if (world->IsValidPosition(x, y)) {
                world->SetMaterial(x, y, material);
            }
        }
    }
}

void MaterialBrush::ApplyPlusBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material) {
    // Thick horizontal line (3 pixels thick)
    for (int dx = -m_size; dx <= m_size; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (world->IsValidPosition(x, y)) {
                world->SetMaterial(x, y, material);
            }
        }
    }
    // Thick vertical line (3 pixels thick)
    for (int dy = -m_size; dy <= m_size; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (world->IsValidPosition(x, y)) {
                world->SetMaterial(x, y, material);
            }
        }
    }
}

} // namespace BGE