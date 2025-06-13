#include "MaterialBrush.h"
#include "../../Simulation/SimulationWorld.h"
#include <algorithm>

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
    }
}

void MaterialBrush::Sample(SimulationWorld* world, int x, int y) {
    if (!world || !world->IsValidPosition(x, y)) return;
    
    m_currentMaterial = world->GetMaterial(x, y);
    m_temperature = world->GetTemperature(x, y);
}

bool MaterialBrush::IsInBrushArea(int centerX, int centerY, int testX, int testY) const {
    switch (m_shape) {
        case BrushShape::Circle: {
            int dx = testX - centerX;
            int dy = testY - centerY;
            return (dx * dx + dy * dy) <= (m_size * m_size);
        }
        case BrushShape::Square: {
            int dx = std::abs(testX - centerX);
            int dy = std::abs(testY - centerY);
            return dx <= m_size && dy <= m_size;
        }
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
                    if (material != MATERIAL_EMPTY) {
                        world->SetTemperature(x, y, m_temperature);
                    }
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
                if (material != MATERIAL_EMPTY) {
                    world->SetTemperature(x, y, m_temperature);
                }
            }
        }
    }
}

} // namespace BGE