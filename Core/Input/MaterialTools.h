#pragma once

#include "MaterialBrush.h"
#include "MaterialPalette.h"
#include "../Math/Vector2.h"

namespace BGE {

class SimulationWorld;
class InputManager;

enum class ToolMode {
    Paint,
    Erase,
    Sample  // Eyedropper
};

class MaterialTools {
public:
    MaterialTools();
    ~MaterialTools() = default;
    
    bool Initialize(SimulationWorld* world);
    void Shutdown();
    void Update(float deltaTime);
    
    // Tool state
    void SetToolMode(ToolMode mode) { m_toolMode = mode; }
    ToolMode GetToolMode() const { return m_toolMode; }
    
    // Input handling
    void OnMousePressed(int button, float x, float y);
    void OnMouseReleased(int button, float x, float y);
    void OnMouseMoved(float x, float y);
    void OnKeyPressed(int key);
    
    // Coordinate conversion (screen to simulation)
    void SetViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight);
    void ScreenToSimulation(float screenX, float screenY, int& simX, int& simY) const;
    
    // Tool access
    MaterialBrush& GetBrush() { return m_brush; }
    const MaterialBrush& GetBrush() const { return m_brush; }
    
    MaterialPalette& GetPalette() { return m_palette; }
    const MaterialPalette& GetPalette() const { return m_palette; }
    
    // Brush painting
    void PaintAt(float screenX, float screenY);
    void EraseAt(float screenX, float screenY);
    void SampleAt(float screenX, float screenY);
    
    // Simulation control shortcuts
    void ToggleSimulation();
    void StepSimulation();
    void ResetSimulation();
    
private:
    SimulationWorld* m_world = nullptr;
    MaterialBrush m_brush;
    MaterialPalette m_palette;
    ToolMode m_toolMode = ToolMode::Paint;
    
    // Mouse state
    bool m_leftMouseDown = false;
    bool m_rightMouseDown = false;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
    
    // Viewport for coordinate conversion
    int m_viewportX = 0;
    int m_viewportY = 0;
    int m_viewportWidth = 800;
    int m_viewportHeight = 600;
    
    void ProcessContinuousPainting();
};

} // namespace BGE