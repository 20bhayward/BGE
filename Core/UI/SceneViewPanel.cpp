#include "SceneViewPanel.h"
#include "../Services.h"
#include <imgui.h>
#include <algorithm>

namespace BGE {

SceneViewPanel::SceneViewPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools)
    : Panel(name, PanelDockPosition::Center)
    , m_world(world)
    , m_tools(tools) {
}

void SceneViewPanel::Initialize() {
    // Scene view should not be collapsible and should fill available space
    SetWindowFlags(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void SceneViewPanel::OnRender() {
    // Store focus and hover state
    m_isFocused = ImGui::IsWindowFocused();
    m_isHovered = ImGui::IsWindowHovered();
    
    RenderToolbar();
    
    ImGui::Separator();
    
    RenderViewport();
    
    if (m_showStats || m_showDebugInfo) {
        RenderOverlay();
    }
}

void SceneViewPanel::RenderToolbar() {
    // Play/Pause controls
    bool isPaused = m_world->IsPaused();
    
    if (isPaused) {
        if (ImGui::Button("▶ Play")) {
            m_world->Play();
        }
    } else {
        if (ImGui::Button("⏸ Pause")) {
            m_world->Pause();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏭ Step")) {
        m_world->Step();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏹ Reset")) {
        m_world->Reset();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("🗑 Clear")) {
        m_world->Clear();
    }
    
    // Simulation speed
    ImGui::SameLine();
    ImGui::Text(" Speed:");
    ImGui::SameLine();
    static float simSpeed = 1.0f;
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##Speed", &simSpeed, 0.1f, 3.0f, "%.1f")) {
        m_world->SetSimulationSpeed(simSpeed);
    }
    
    // View options
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &m_showGrid);
    
    ImGui::SameLine();
    ImGui::Checkbox("Stats", &m_showStats);
    
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_showDebugInfo);
    
    // Tool mode
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ToolMode currentMode = m_tools->GetToolMode();
    const char* modeNames[] = { "Paint", "Erase", "Sample" };
    int modeIndex = static_cast<int>(currentMode);
    ImGui::SetNextItemWidth(80);
    if (ImGui::Combo("##Tool", &modeIndex, modeNames, 3)) {
        m_tools->SetToolMode(static_cast<ToolMode>(modeIndex));
    }
    
    // Brush size
    ImGui::SameLine();
    int brushSize = m_tools->GetBrush().GetSize();
    ImGui::SetNextItemWidth(80);
    if (ImGui::SliderInt("##BrushSize", &brushSize, 1, 50)) {
        m_tools->GetBrush().SetSize(brushSize);
    }
}

void SceneViewPanel::RenderViewport() {
    // Get available content region
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Calculate viewport bounds
    m_viewportX = cursorPos.x - windowPos.x;
    m_viewportY = cursorPos.y - windowPos.y;
    m_viewportWidth = contentRegion.x;
    m_viewportHeight = contentRegion.y;
    
    // Create invisible button to capture input
    ImGui::InvisibleButton("SceneViewport", contentRegion);
    
    // Handle input when hovered
    if (ImGui::IsItemHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float relativeX = mousePos.x - cursorPos.x;
        float relativeY = mousePos.y - cursorPos.y;
        
        // Update tools with relative mouse position
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
    
    // Draw grid if enabled
    if (m_showGrid) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 p = cursorPos;
        
        float gridSize = 32.0f;
        ImU32 gridColor = IM_COL32(200, 200, 200, 40);
        
        for (float x = fmod(p.x, gridSize); x < contentRegion.x; x += gridSize) {
            drawList->AddLine(ImVec2(p.x + x, p.y), ImVec2(p.x + x, p.y + contentRegion.y), gridColor);
        }
        for (float y = fmod(p.y, gridSize); y < contentRegion.y; y += gridSize) {
            drawList->AddLine(ImVec2(p.x, p.y + y), ImVec2(p.x + contentRegion.x, p.y + y), gridColor);
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

void SceneViewPanel::RenderOverlay() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    // Position overlay in top-left corner of viewport
    ImVec2 overlayPos = ImVec2(windowPos.x + 10, windowPos.y + 50); // Offset for toolbar
    
    if (m_showStats) {
        // Background for stats
        ImVec2 statsBgSize = ImVec2(200, 120);
        drawList->AddRectFilled(overlayPos, 
                               ImVec2(overlayPos.x + statsBgSize.x, overlayPos.y + statsBgSize.y),
                               IM_COL32(0, 0, 0, 128), 4.0f);
        
        // Stats text
        ImVec2 textPos = ImVec2(overlayPos.x + 8, overlayPos.y + 8);
        ImU32 textColor = IM_COL32(255, 255, 255, 255);
        
        char buffer[256];
        sprintf(buffer, "Status: %s", m_world->IsPaused() ? "PAUSED" : "RUNNING");
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        sprintf(buffer, "Frame: %I64d", static_cast<long long>(m_world->GetUpdateCount()));
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        sprintf(buffer, "Active Cells: %d", m_world->GetActiveCells());
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        sprintf(buffer, "Update Time: %.2fms", m_world->GetLastUpdateTime() * 1000.0f);
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        sprintf(buffer, "World: %dx%d", m_world->GetWidth(), m_world->GetHeight());
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        ToolMode currentMode = m_tools->GetToolMode();
        sprintf(buffer, "Tool: %s (Size: %d)", 
                currentMode == ToolMode::Paint ? "Paint" : 
                currentMode == ToolMode::Erase ? "Erase" : "Sample",
                m_tools->GetBrush().GetSize());
        drawList->AddText(textPos, textColor, buffer);
        
        overlayPos.y += statsBgSize.y + 10;
    }
    
    if (m_showDebugInfo) {
        // Debug info overlay
        ImVec2 debugBgSize = ImVec2(250, 80);
        drawList->AddRectFilled(overlayPos, 
                               ImVec2(overlayPos.x + debugBgSize.x, overlayPos.y + debugBgSize.y),
                               IM_COL32(128, 0, 0, 128), 4.0f);
        
        ImVec2 textPos = ImVec2(overlayPos.x + 8, overlayPos.y + 8);
        ImU32 textColor = IM_COL32(255, 255, 255, 255);
        
        char buffer[256];
        sprintf(buffer, "Viewport: %.0fx%.0f", m_viewportWidth, m_viewportHeight);
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        sprintf(buffer, "Mouse: %.0f, %.0f", ImGui::GetMousePos().x, ImGui::GetMousePos().y);
        drawList->AddText(textPos, textColor, buffer);
        
        textPos.y += 16;
        sprintf(buffer, "Hovered: %s, Focused: %s", m_isHovered ? "Yes" : "No", m_isFocused ? "Yes" : "No");
        drawList->AddText(textPos, textColor, buffer);
    }
}

} // namespace BGE