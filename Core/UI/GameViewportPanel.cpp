#include "GameViewportPanel.h"
#include "../Services.h"
#include "../Logger.h"
#include "../../Renderer/Renderer.h"
#include <imgui.h>
#include <algorithm>

namespace BGE {

GameViewportPanel::GameViewportPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools)
    : Panel(name, PanelDockPosition::Center)
    , m_world(world)
    , m_tools(tools) {
}

void GameViewportPanel::Initialize() {
    // Game viewport should fill available space and be the central focus
    SetWindowFlags(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void GameViewportPanel::OnRender() {
    // Store focus and hover state
    m_isFocused = ImGui::IsWindowFocused();
    m_isHovered = ImGui::IsWindowHovered();
    
    // Main game content (no toolbar - now in DebugToolbarPanel)
    RenderGameContent();
    
    if (m_showStats) {
        RenderOverlayStats();
    }
}

void GameViewportPanel::RenderViewportToolbar() {
    // Compact toolbar with essential controls only
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    
    // Play controls group
    bool isPaused = m_world->IsPaused();
    
    if (isPaused) {
        if (ImGui::Button("▶")) {
            m_world->Play();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play (P)");
    } else {
        if (ImGui::Button("⏸")) {
            m_world->Pause();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause (P)");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏭")) {
        m_world->Step();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step (S)");
    
    ImGui::SameLine();
    if (ImGui::Button("⏹")) {
        m_world->Reset();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset (R)");
    
    // Separator
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Simulation speed - compact
    static float simSpeed = 1.0f;
    ImGui::SetNextItemWidth(80);
    if (ImGui::SliderFloat("##Speed", &simSpeed, 0.1f, 3.0f, "%.1fx")) {
        m_world->SetSimulationSpeed(simSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Simulation Speed");
    
    // Tool selection - compact
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    ToolMode currentMode = m_tools->GetToolMode();
    const char* toolIcons[] = { "🖌", "🗑", "🔍" };
    const char* toolNames[] = { "Paint", "Erase", "Sample" };
    int modeIndex = static_cast<int>(currentMode);
    
    ImGui::SetNextItemWidth(60);
    if (ImGui::Combo("##Tool", &modeIndex, toolNames, 3)) {
        m_tools->SetToolMode(static_cast<ToolMode>(modeIndex));
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tool Mode");
    
    // Brush size - compact
    ImGui::SameLine();
    int brushSize = m_tools->GetBrush().GetSize();
    ImGui::SetNextItemWidth(60);
    if (ImGui::SliderInt("##Size", &brushSize, 1, 20, "%d")) {
        m_tools->GetBrush().SetSize(brushSize);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Brush Size");
    
    // View options on the right
    ImGui::SameLine();
    float rightOffset = ImGui::GetContentRegionAvail().x - 120;
    if (rightOffset > 0) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightOffset);
    }
    
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Stats", &m_showStats);
    
    ImGui::PopStyleVar(2);
}

void GameViewportPanel::RenderGameContent() {
    // Get available content region for the game
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Calculate viewport bounds in screen space (absolute coordinates)
    // ImGui cursor position is relative to the main viewport, we need absolute screen coordinates
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    m_viewportX = cursorPos.x - viewport->WorkPos.x;
    m_viewportY = cursorPos.y - viewport->WorkPos.y;
    m_viewportWidth = contentRegion.x;
    m_viewportHeight = contentRegion.y;
    
    // Set the renderer viewport to this area so the game renders here
    auto renderer = Services::GetRenderer();
    auto world = Services::GetWorld();
    if (renderer) {
        renderer->SetSimulationViewport(static_cast<int>(m_viewportX), static_cast<int>(m_viewportY), 
                                       static_cast<int>(m_viewportWidth), static_cast<int>(m_viewportHeight));
        
        // Debug logging to verify viewport is being set correctly
        static int logCounter = 0;
        if (logCounter++ % 60 == 0) { // Log once per second at 60fps
            BGE_LOG_INFO("GameViewport", "Setting viewport to (" + std::to_string(static_cast<int>(m_viewportX)) + 
                        "," + std::to_string(static_cast<int>(m_viewportY)) + ") size " + 
                        std::to_string(static_cast<int>(m_viewportWidth)) + "x" + 
                        std::to_string(static_cast<int>(m_viewportHeight)));
            
            // Debug: Check if world has content
            if (world) {
                uint32_t activeCells = world->GetActiveCells();
                const uint8_t* pixelData = world->GetPixelData();
                BGE_LOG_INFO("GameViewport", "World has " + std::to_string(activeCells) + " active cells, pixel data: " + 
                            (pixelData ? "available" : "null"));
                
                // Check if any pixels are non-transparent
                if (pixelData) {
                    int nonTransparentPixels = 0;
                    uint32_t totalPixels = world->GetWidth() * world->GetHeight();
                    for (uint32_t i = 0; i < totalPixels * 4; i += 4) {
                        if (pixelData[i + 3] > 0) { // Check alpha channel
                            nonTransparentPixels++;
                        }
                    }
                    BGE_LOG_INFO("GameViewport", "Found " + std::to_string(nonTransparentPixels) + " non-transparent pixels");
                }
            }
        }
    }
    
    // Draw a visible background to show the viewport area
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 bgColor = IM_COL32(45, 45, 48, 255); // Dark gray background
    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), bgColor);
    
    // Add a border to clearly show the game viewport area
    ImU32 borderColor = m_cameraMode ? IM_COL32(0, 150, 255, 255) : IM_COL32(100, 100, 100, 255); // Blue border in camera mode
    drawList->AddRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), borderColor, 0.0f, 0, 2.0f);
    
    // Camera mode indicator
    if (m_cameraMode) {
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), IM_COL32(0, 150, 255, 255), "CAMERA MODE (C to toggle)");
    }
    
    // Create invisible button to capture input for the game
    ImGui::InvisibleButton("GameViewport", contentRegion);
    
    // Handle input when hovered
    if (ImGui::IsItemHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float relativeX = mousePos.x - cursorPos.x;
        float relativeY = mousePos.y - cursorPos.y;
        
        // Handle camera controls first
        HandleCameraInput(relativeX, relativeY);
        
        // Then handle material tools if not in camera mode
        if (!m_cameraMode) {
            m_tools->OnMouseMoved(relativeX, relativeY);
            
            // Handle mouse clicks
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_tools->OnMousePressed(0, relativeX, relativeY);
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                m_tools->OnMousePressed(1, relativeX, relativeY);
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                m_tools->OnMouseReleased(0, relativeX, relativeY);
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                m_tools->OnMouseReleased(1, relativeX, relativeY);
            }
        }
        
        // Handle keyboard shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_C)) {
            m_cameraMode = !m_cameraMode;
        }
    }
    
    // Draw grid overlay if enabled
    if (m_showGrid) {
        ImDrawList* gridDrawList = ImGui::GetWindowDrawList();
        ImVec2 p = cursorPos;
        
        float gridSize = 32.0f;
        ImU32 gridColor = IM_COL32(200, 200, 200, 40);
        
        for (float x = fmod(p.x, gridSize); x < contentRegion.x; x += gridSize) {
            gridDrawList->AddLine(ImVec2(p.x + x, p.y), ImVec2(p.x + x, p.y + contentRegion.y), gridColor);
        }
        for (float y = fmod(p.y, gridSize); y < contentRegion.y; y += gridSize) {
            gridDrawList->AddLine(ImVec2(p.x, p.y + y), ImVec2(p.x + contentRegion.x, p.y + y), gridColor);
        }
    }
    
    // Drag and drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_ID")) {
            MaterialID materialId = *(MaterialID*)payload->Data;
            m_tools->GetBrush().SetMaterial(materialId);
        }
        ImGui::EndDragDropTarget();
    }
}

void GameViewportPanel::RenderOverlayStats() {
    // Overlay stats in bottom-left corner
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | 
                                   ImGuiWindowFlags_NoNav;
    
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    
    window_pos.x = work_pos.x + PAD;
    window_pos.y = work_pos.y + work_size.y - PAD;
    window_pos_pivot.x = 0.0f;
    window_pos_pivot.y = 1.0f;
    
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f);
    
    if (ImGui::Begin("Viewport Stats", nullptr, window_flags)) {
        ImGui::Text("Status: %s", m_world->IsPaused() ? "PAUSED" : "RUNNING");
        ImGui::Text("Frame: %I64d", static_cast<long long>(m_world->GetUpdateCount()));
        ImGui::Text("Particles: %d", m_world->GetActiveCells());
        ImGui::Text("FPS: %.1f (%.2fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        
        ToolMode currentMode = m_tools->GetToolMode();
        const char* toolName = (currentMode == ToolMode::Paint) ? "Paint" : 
                              (currentMode == ToolMode::Erase) ? "Erase" : "Sample";
        ImGui::Text("Tool: %s (%d)", toolName, m_tools->GetBrush().GetSize());
    }
    ImGui::End();
}

void GameViewportPanel::HandleCameraInput(float mouseX, float mouseY) {
    auto renderer = Services::GetRenderer();
    if (!renderer || !renderer->GetPixelCamera()) return;
    
    auto* camera = renderer->GetPixelCamera();
    
    // Mouse wheel zoom
    float mouseWheel = ImGui::GetIO().MouseWheel;
    if (mouseWheel != 0.0f) {
        int currentZoom = camera->GetZoom();
        int newZoom = std::max(1, std::min(32, currentZoom + static_cast<int>(mouseWheel)));
        camera->SetZoom(newZoom);
    }
    
    // Camera panning with middle mouse or when in camera mode
    bool shouldPan = (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) || 
                     (m_cameraMode && ImGui::IsMouseDown(ImGuiMouseButton_Left));
    
    if (shouldPan) {
        if (!m_dragging) {
            m_dragging = true;
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        } else {
            float deltaX = mouseX - m_lastMouseX;
            float deltaY = mouseY - m_lastMouseY;
            
            // Scale movement by zoom level (more zoom = slower movement)
            float moveScale = 1.0f / camera->GetZoom();
            deltaX *= moveScale;
            deltaY *= moveScale;
            
            Vector2 currentPos = camera->GetPosition();
            camera->SetPosition(Vector2{currentPos.x - deltaX, currentPos.y + deltaY}); // Y flipped for screen coords
            
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        }
    } else {
        m_dragging = false;
    }
    
    // Camera reset with middle click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
        camera->SetPosition(Vector2{0, 0});
        camera->SetZoom(1);
    }
}

} // namespace BGE