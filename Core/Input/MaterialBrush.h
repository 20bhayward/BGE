#pragma once

#include "../../Simulation/Materials/Material.h"
#include "../Math/Vector2.h"

namespace BGE {

class SimulationWorld;

enum class BrushShape {
    Circle,
    Square,
    Triangle,
    Diamond,
    Line,
    Cross,
    Star,
    Plus
};

class MaterialBrush {
public:
    MaterialBrush();
    ~MaterialBrush() = default;
    
    // Brush properties
    void SetMaterial(MaterialID material) { m_currentMaterial = material; }
    MaterialID GetMaterial() const { return m_currentMaterial; }
    
    void SetSize(int size) { m_size = size; }
    int GetSize() const { return m_size; }
    
    void SetShape(BrushShape shape) { m_shape = shape; }
    BrushShape GetShape() const { return m_shape; }
    
    
    // Brush operations
    void Paint(SimulationWorld* world, int x, int y);
    void Erase(SimulationWorld* world, int x, int y);
    void Sample(SimulationWorld* world, int x, int y); // Eyedropper tool
    
    // Brush preview
    bool IsInBrushArea(int centerX, int centerY, int testX, int testY) const;
    
private:
    MaterialID m_currentMaterial = MATERIAL_EMPTY;
    int m_size = 5;
    BrushShape m_shape = BrushShape::Circle;
    
    void ApplyCircleBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplySquareBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplyTriangleBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplyDiamondBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplyLineBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplyCrossBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplyStarBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
    void ApplyPlusBrush(SimulationWorld* world, int centerX, int centerY, MaterialID material);
};

} // namespace BGE