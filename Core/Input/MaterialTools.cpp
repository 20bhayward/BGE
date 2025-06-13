#include "MaterialTools.h"
#include "../../Simulation/SimulationWorld.h"
#include <iostream>

namespace BGE {

MaterialTools::MaterialTools() = default;

bool MaterialTools::Initialize(SimulationWorld* world) {
    if (!world) {
        std::cerr << "MaterialTools: Invalid world pointer" << std::endl;
        return false;
    }
    
    m_world = world;
    
    // Initialize palette with the world's material system
    m_palette.Initialize(world->GetMaterialSystem());
    
    // Set initial brush material to first palette item (sand)
    if (m_palette.GetMaterialCount() > 1) {
        m_palette.SelectMaterial(1); // Skip eraser, select sand
        m_brush.SetMaterial(m_palette.GetSelectedMaterial());
    }
    
    std::cout << "MaterialTools initialized with " << m_palette.GetMaterialCount() << " materials" << std::endl;
    return true;
}

void MaterialTools::Shutdown() {
    m_world = nullptr;
}

void MaterialTools::Update(float deltaTime) {
    (void)deltaTime;
    
    // Process continuous painting while mouse is held down
    ProcessContinuousPainting();
}

void MaterialTools::OnMousePressed(int button, float x, float y) {
    m_lastMouseX = x;
    m_lastMouseY = y;
    
    if (button == 0) { // Left mouse button
        m_leftMouseDown = true;
        
        switch (m_toolMode) {
            case ToolMode::Paint:
                PaintAt(x, y);
                break;
            case ToolMode::Erase:
                EraseAt(x, y);
                break;
            case ToolMode::Sample:
                SampleAt(x, y);
                break;
        }
    }
    else if (button == 1) { // Right mouse button
        m_rightMouseDown = true;
        // Right click always erases
        EraseAt(x, y);
    }
}

void MaterialTools::OnMouseReleased(int button, float x, float y) {
    (void)x; (void)y;
    
    if (button == 0) {
        m_leftMouseDown = false;
    }
    else if (button == 1) {
        m_rightMouseDown = false;
    }
}

void MaterialTools::OnMouseMoved(float x, float y) {
    m_lastMouseX = x;
    m_lastMouseY = y;
}

void MaterialTools::OnKeyPressed(int key) {
    // Check for material hotkeys
    m_palette.SelectMaterialByHotkey(key);
    m_brush.SetMaterial(m_palette.GetSelectedMaterial());
    
    // Tool mode shortcuts
    switch (key) {
        case 'B': case 'b': // Brush tool
            SetToolMode(ToolMode::Paint);
            break;
        case 'E': case 'e': // Eraser tool
            SetToolMode(ToolMode::Erase);
            break;
        case 'I': case 'i': // Sample/eyedropper tool
            SetToolMode(ToolMode::Sample);
            break;
        case 'P': case 'p': // Pause/play simulation
            ToggleSimulation();
            break;
        case 'S': case 's': // Step simulation
            StepSimulation();
            break;
        case 'R': case 'r': // Reset simulation
            ResetSimulation();
            break;
        case '[': // Decrease brush size
            m_brush.SetSize(std::max(1, m_brush.GetSize() - 1));
            break;
        case ']': // Increase brush size
            m_brush.SetSize(std::min(20, m_brush.GetSize() + 1));
            break;
    }
}

void MaterialTools::SetViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight) {
    m_viewportX = viewportX;
    m_viewportY = viewportY;
    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;
}

void MaterialTools::ScreenToSimulation(float screenX, float screenY, int& simX, int& simY) const {
    if (!m_world) {
        simX = simY = 0;
        return;
    }
    
    // Convert screen coordinates to simulation coordinates
    float relativeX = (screenX - m_viewportX) / m_viewportWidth;
    float relativeY = (screenY - m_viewportY) / m_viewportHeight;
    
    simX = static_cast<int>(relativeX * m_world->GetWidth());
    simY = static_cast<int>(relativeY * m_world->GetHeight());
}

void MaterialTools::PaintAt(float screenX, float screenY) {
    if (!m_world) return;
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    m_brush.Paint(m_world, simX, simY);
}

void MaterialTools::EraseAt(float screenX, float screenY) {
    if (!m_world) return;
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    m_brush.Erase(m_world, simX, simY);
}

void MaterialTools::SampleAt(float screenX, float screenY) {
    if (!m_world) return;
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    m_brush.Sample(m_world, simX, simY);
    
    // Update palette selection to match sampled material
    m_palette.SelectMaterialByID(m_brush.GetMaterial());
    
    std::cout << "Sampled material ID: " << m_brush.GetMaterial() 
              << " at (" << simX << ", " << simY << ")" << std::endl;
}

void MaterialTools::ToggleSimulation() {
    if (!m_world) return;
    
    m_world->TogglePause();
    std::cout << "Simulation " << (m_world->IsPaused() ? "PAUSED" : "PLAYING") << std::endl;
}

void MaterialTools::StepSimulation() {
    if (!m_world) return;
    
    m_world->Step();
    std::cout << "Simulation stepped one frame" << std::endl;
}

void MaterialTools::ResetSimulation() {
    if (!m_world) return;
    
    m_world->Reset();
    std::cout << "Simulation reset" << std::endl;
}

void MaterialTools::ProcessContinuousPainting() {
    if (!m_leftMouseDown && !m_rightMouseDown) return;
    
    if (m_leftMouseDown) {
        switch (m_toolMode) {
            case ToolMode::Paint:
                PaintAt(m_lastMouseX, m_lastMouseY);
                break;
            case ToolMode::Erase:
                EraseAt(m_lastMouseX, m_lastMouseY);
                break;
            case ToolMode::Sample:
                // Sampling doesn't need continuous operation
                break;
        }
    }
    
    if (m_rightMouseDown) {
        EraseAt(m_lastMouseX, m_lastMouseY);
    }
}

} // namespace BGE