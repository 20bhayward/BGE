#include "MaterialTools.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include "../Logger.h"
#include <iostream>
#include <algorithm>

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
    
    // Initialize categorized palette
    m_categorizedPalette.Initialize(world->GetMaterialSystem());
    
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
            case ToolMode::Info:
                InspectAt(x, y);
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
    
    // Inspect material for info display if inspector is enabled
    if (m_inspectorEnabled) {
        InspectAt(x, y);
    }
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
        case 'Q': case 'q': // Toggle inspector (instead of Info-only mode)
            SetInspectorEnabled(!IsInspectorEnabled());
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
            m_brush.SetSize(std::min(100, m_brush.GetSize() + 1));
            break;
    }
}

void MaterialTools::SetViewport(int viewportX, int viewportY, int viewportWidth, int viewportHeight) {
    m_viewportX = viewportX;
    m_viewportY = viewportY;
    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;
    
    BGE_LOG_INFO("MaterialTools", "Viewport set to: (" + std::to_string(viewportX) + "," + 
                 std::to_string(viewportY) + ") size " + std::to_string(viewportWidth) + "x" + 
                 std::to_string(viewportHeight));
}

void MaterialTools::ScreenToSimulation(float screenX, float screenY, int& simX, int& simY) const {
    if (!m_world) {
        simX = simY = 0;
        return;
    }
    
    // Check if the mouse is within the simulation viewport
    if (screenX < m_viewportX || screenX >= m_viewportX + m_viewportWidth ||
        screenY < m_viewportY || screenY >= m_viewportY + m_viewportHeight) {
        simX = simY = -1; // Out of bounds
        return;
    }
    
    // Convert screen coordinates to viewport-relative coordinates
    float relativeX = (screenX - m_viewportX) / m_viewportWidth;
    float relativeY = (screenY - m_viewportY) / m_viewportHeight;
    
    // Don't flip Y here - the OpenGL viewport transformation in the renderer
    // already handles the coordinate system conversion
    
    // Clamp to valid range
    relativeX = std::max(0.0f, std::min(1.0f, relativeX));
    relativeY = std::max(0.0f, std::min(1.0f, relativeY));
    
    // Convert to simulation coordinates
    simX = static_cast<int>(relativeX * m_world->GetWidth());
    simY = static_cast<int>(relativeY * m_world->GetHeight());
    
    // Clamp to simulation bounds
    simX = std::max(0, std::min(static_cast<int>(m_world->GetWidth()) - 1, simX));
    simY = std::max(0, std::min(static_cast<int>(m_world->GetHeight()) - 1, simY));
    
    // Debug coordinate conversion occasionally
    static int conversionCounter = 0;
    if (++conversionCounter % 30 == 0) { // Log every 30th conversion
        BGE_LOG_INFO("MaterialTools", "Coordinate conversion: screen(" + 
                     std::to_string(screenX) + "," + std::to_string(screenY) + ") → viewport(" +
                     std::to_string(relativeX) + "," + std::to_string(relativeY) + ") → sim(" +
                     std::to_string(simX) + "," + std::to_string(simY) + ")");
    }
}

void MaterialTools::PaintAt(float screenX, float screenY) {
    if (!m_world) return;
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    // Skip if coordinates are out of bounds
    if (simX < 0 || simY < 0) return;
    
    m_brush.Paint(m_world, simX, simY);
}

void MaterialTools::EraseAt(float screenX, float screenY) {
    if (!m_world) return;
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    // Skip if coordinates are out of bounds
    if (simX < 0 || simY < 0) return;
    
    m_brush.Erase(m_world, simX, simY);
}

void MaterialTools::SampleAt(float screenX, float screenY) {
    if (!m_world) return;
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    // Skip if coordinates are out of bounds
    if (simX < 0 || simY < 0) return;
    
    m_brush.Sample(m_world, simX, simY);
    
    // Update palette selection to match sampled material
    m_palette.SelectMaterialByID(m_brush.GetMaterial());
    
    std::cout << "Sampled material ID: " << m_brush.GetMaterial() 
              << " at (" << simX << ", " << simY << ")" << std::endl;
}

void MaterialTools::InspectAt(float screenX, float screenY) {
    if (!m_world) {
        m_inspectedMaterial.hasData = false;
        return;
    }
    
    int simX, simY;
    ScreenToSimulation(screenX, screenY, simX, simY);
    
    // Skip if coordinates are out of bounds
    if (simX < 0 || simY < 0) {
        m_inspectedMaterial.hasData = false;
        return;
    }
    
    // Get cell data
    const Cell& cell = m_world->GetCell(simX, simY);
    MaterialSystem* materialSystem = m_world->GetMaterialSystem();
    
    if (!materialSystem) {
        m_inspectedMaterial.hasData = false;
        return;
    }
    
    // Fill in material info
    m_inspectedMaterial.hasData = true;
    m_inspectedMaterial.materialID = cell.material;
    m_inspectedMaterial.temperature = cell.temperature;
    m_inspectedMaterial.posX = simX;
    m_inspectedMaterial.posY = simY;
    
    if (cell.material == MATERIAL_EMPTY) {
        m_inspectedMaterial.name = "Empty";
        m_inspectedMaterial.description = "No material present";
        m_inspectedMaterial.density = 0.0f;
        m_inspectedMaterial.viscosity = 0.0f;
        m_inspectedMaterial.reactions.clear();
    } else {
        const Material* material = materialSystem->GetMaterialPtr(cell.material);
        if (material) {
            m_inspectedMaterial.name = material->GetName();
            
            // Get description from palette if available
            const PaletteMaterial* paletteMat = m_palette.GetMaterialByID(cell.material);
            m_inspectedMaterial.description = paletteMat ? paletteMat->description : "";
            
            m_inspectedMaterial.density = material->GetPhysicalProps().density;
            m_inspectedMaterial.viscosity = material->GetPhysicalProps().viscosity;
            
            // Get reaction information
            m_inspectedMaterial.reactions.clear();
            const auto& reactions = material->GetReactions();
            for (const auto& reaction : reactions) {
                const Material* reactantMat = materialSystem->GetMaterialPtr(reaction.reactant);
                const Material* product1Mat = materialSystem->GetMaterialPtr(reaction.product1);
                
                std::string reactionDesc = "With " + (reactantMat ? reactantMat->GetName() : "Unknown");
                reactionDesc += " -> " + (product1Mat ? product1Mat->GetName() : "Unknown");
                
                if (reaction.product2 != MATERIAL_EMPTY) {
                    const Material* product2Mat = materialSystem->GetMaterialPtr(reaction.product2);
                    reactionDesc += " + " + (product2Mat ? product2Mat->GetName() : "Unknown");
                }
                
                reactionDesc += " (" + std::to_string((int)(reaction.probability * 100)) + "%)";
                m_inspectedMaterial.reactions.push_back(reactionDesc);
            }
        } else {
            m_inspectedMaterial.name = "Unknown Material";
            m_inspectedMaterial.description = "Material data not found";
            m_inspectedMaterial.density = 0.0f;
            m_inspectedMaterial.viscosity = 0.0f;
            m_inspectedMaterial.reactions.clear();
        }
    }
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
            case ToolMode::Info:
                // Info mode doesn't paint, just inspects (already handled in OnMouseMoved)
                break;
        }
    }
    
    if (m_rightMouseDown) {
        EraseAt(m_lastMouseX, m_lastMouseY);
    }
}

} // namespace BGE