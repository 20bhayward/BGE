#include "SculptingPanel.h"
#include "../../Services.h"
#include "../../Logger.h"
#include "../../../Renderer/Renderer.h"
#include "../../Components.h"
#include "../../Entity.h"
#include <imgui.h>
#include <algorithm>
#include <filesystem>

namespace BGE {

SculptingPanel::SculptingPanel(const std::string& name, SimulationWorld* world, MaterialTools* tools)
    : Panel(name, PanelDockPosition::Center)
    , m_world(world)
    , m_tools(tools) {
}

void SculptingPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void SculptingPanel::OnRender() {
    m_isFocused = ImGui::IsWindowFocused();
    m_isHovered = ImGui::IsWindowHovered();
    
    RenderSculptingToolbar();
    RenderSculptingContent();
}

void SculptingPanel::RenderSculptingToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    
    // Simulation controls
    bool isPaused = m_world->IsPaused();
    
    if (isPaused) {
        if (ImGui::Button(" Play ")) {
            m_world->Play();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play Simulation (P)");
    } else {
        if (ImGui::Button("Pause")) {
            m_world->Pause();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause Simulation (P)");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        m_world->Step();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step Simulation (S)");
    
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_world->Reset();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset Simulation (R)");
    
    // Separator
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Simulation speed
    static float simSpeed = 1.0f;
    ImGui::SetNextItemWidth(80);
    if (ImGui::SliderFloat("##Speed", &simSpeed, 0.1f, 3.0f, "%.1fx")) {
        m_world->SetSimulationSpeed(simSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Simulation Speed");
    
    // Tool selection
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    ToolMode currentMode = m_tools->GetToolMode();
    const char* toolNames[] = { "Paint", "Erase", "Sample" };
    int modeIndex = static_cast<int>(currentMode);
    
    ImGui::SetNextItemWidth(80);
    if (ImGui::Combo("##Tool", &modeIndex, toolNames, 3)) {
        m_tools->SetToolMode(static_cast<ToolMode>(modeIndex));
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Material Tool");
    
    // Brush size
    ImGui::SameLine();
    int brushSize = m_tools->GetBrush().GetSize();
    ImGui::SetNextItemWidth(60);
    if (ImGui::SliderInt("##Size", &brushSize, 1, 20, "%d")) {
        m_tools->GetBrush().SetSize(brushSize);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Brush Size");
    
    // View options on the right
    ImGui::SameLine();
    float rightOffset = ImGui::GetContentRegionAvail().x - 150;
    if (rightOffset > 0) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightOffset);
    }
    
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Preview", &m_showMaterialPreview);
    
    ImGui::PopStyleVar(2);
}

void SculptingPanel::RenderSculptingContent() {
    // Get available content region for sculpting
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Calculate viewport bounds in screen space
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    m_viewportX = cursorPos.x - viewport->WorkPos.x;
    m_viewportY = cursorPos.y - viewport->WorkPos.y;
    m_viewportWidth = contentRegion.x;
    m_viewportHeight = contentRegion.y;
    
    auto renderer = Services::GetRenderer();
    auto world = Services::GetWorld();
    
    if (renderer && world && m_viewportWidth > 0 && m_viewportHeight > 0) {
        // Create or recreate framebuffer if size changed
        int currentTexWidth, currentTexHeight;
        renderer->GetGameTextureSize(currentTexWidth, currentTexHeight);
        
        if (currentTexWidth != static_cast<int>(m_viewportWidth) || 
            currentTexHeight != static_cast<int>(m_viewportHeight) ||
            renderer->GetGameTextureId() == 0) {
            
            bool success = renderer->CreateGameFramebuffer(static_cast<int>(m_viewportWidth), 
                                                          static_cast<int>(m_viewportHeight));
            if (!success) {
                BGE_LOG_ERROR("SculptingPanel", "Failed to create sculpting framebuffer");
                return;
            }
        }
        
        // Update MaterialTools viewport
        m_tools->SetViewport(static_cast<int>(cursorPos.x), static_cast<int>(cursorPos.y), 
                            static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y));
        
        // Render the world for sculpting
        renderer->BeginRenderToTexture();
        renderer->BeginFrame();
        renderer->RenderWorld(world.get());
        renderer->RenderParticles();
        renderer->EndFrame();
        renderer->EndRenderToTexture();
        
        // Display the rendered texture
        uint32_t textureId = renderer->GetGameTextureId();
        if (textureId != 0) {
            ImTextureID imguiTextureId = static_cast<ImTextureID>(static_cast<uintptr_t>(textureId));
            ImGui::Image(imguiTextureId, contentRegion, ImVec2(0, 1), ImVec2(1, 0));
        } else {
            // Fallback placeholder
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImU32 bgColor = IM_COL32(60, 40, 30, 255);
            drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), bgColor);
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), 
                             IM_COL32(255, 255, 255, 255), "Sculpting Mode");
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 30), 
                             IM_COL32(200, 200, 200, 255), "Material editing and world sculpting");
        }
        
        // Add sculpting-specific border
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 borderColor = m_isFocused ? IM_COL32(150, 100, 50, 150) : IM_COL32(100, 100, 100, 50);
        drawList->AddRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), 
                         borderColor, 0.0f, 0, 1.0f);
        
        // Camera mode indicator
        if (m_cameraMode) {
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), 
                             IM_COL32(150, 100, 50, 255), "CAMERA MODE - WASD to move, C to exit");
        } else {
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + contentRegion.y - 25), 
                             IM_COL32(150, 150, 150, 150), "Press C for camera mode | Left click to sculpt");
        }
        
        // Handle input when image is hovered
        bool imageHovered = ImGui::IsItemHovered();
        if (imageHovered) {
            HandleCameraInput(ImGui::GetMousePos().x - cursorPos.x, ImGui::GetMousePos().y - cursorPos.y);
            HandleMaterialInput(cursorPos, contentRegion);
            HandleMaterialDragAndDrop(ImGui::GetMousePos(), contentRegion);
        }
        
        // Handle keyboard input when focused
        if (m_isFocused || imageHovered) {
            if (ImGui::IsKeyPressed(static_cast<ImGuiKey>('C'))) {
                m_cameraMode = !m_cameraMode;
            }
        }
    } else {
        // Fallback when no renderer available
        ImGui::InvisibleButton("SculptingViewport", contentRegion);
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 bgColor = IM_COL32(60, 40, 30, 255);
        drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), bgColor);
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), 
                         IM_COL32(255, 255, 255, 255), "Sculpting Panel");
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 30), 
                         IM_COL32(200, 200, 200, 255), "Material editing and world sculpting");
    }
    
    // Draw grid overlay if enabled
    if (m_showGrid) {
        ImDrawList* gridDrawList = ImGui::GetWindowDrawList();
        ImVec2 p = cursorPos;
        
        float gridSize = 32.0f;
        ImU32 gridColor = IM_COL32(150, 100, 50, 60);
        
        for (float x = fmod(p.x, gridSize); x < contentRegion.x; x += gridSize) {
            gridDrawList->AddLine(ImVec2(p.x + x, p.y), ImVec2(p.x + x, p.y + contentRegion.y), gridColor);
        }
        for (float y = fmod(p.y, gridSize); y < contentRegion.y; y += gridSize) {
            gridDrawList->AddLine(ImVec2(p.x, p.y + y), ImVec2(p.x + contentRegion.x, p.y + y), gridColor);
        }
    }
    
    // Drag and drop target for materials
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_ID")) {
            MaterialID materialId = *(MaterialID*)payload->Data;
            m_tools->GetBrush().SetMaterial(materialId);
        }
        ImGui::EndDragDropTarget();
    }
}

void SculptingPanel::HandleCameraInput(float mouseX, float mouseY) {
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
    
    // Camera panning with middle mouse or in camera mode
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
            
            float moveScale = 1.0f / camera->GetZoom();
            deltaX *= moveScale;
            deltaY *= moveScale;
            
            Vector2 currentPos = camera->GetPosition();
            camera->SetPosition(Vector2{currentPos.x - deltaX, currentPos.y + deltaY});
            
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        }
    } else {
        m_dragging = false;
    }
}

void SculptingPanel::HandleMaterialInput(ImVec2 cursorPos, ImVec2 contentRegion) {
    if (!m_tools || m_cameraMode) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    float relativeX = mousePos.x - cursorPos.x;
    float relativeY = mousePos.y - cursorPos.y;
    
    // Ensure mouse is within bounds
    if (relativeX < 0 || relativeY < 0 || relativeX >= contentRegion.x || relativeY >= contentRegion.y) {
        return;
    }
    
    // Handle material tools
    m_tools->OnMouseMoved(mousePos.x, mousePos.y);
    
    // Handle mouse clicks for material editing
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_tools->OnMousePressed(0, mousePos.x, mousePos.y);
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_tools->OnMousePressed(1, mousePos.x, mousePos.y);
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_tools->OnMouseReleased(0, mousePos.x, mousePos.y);
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        m_tools->OnMouseReleased(1, mousePos.x, mousePos.y);
    }
}

void SculptingPanel::HandleMaterialDragAndDrop(ImVec2 /*mousePos*/, ImVec2 /*contentRegion*/) {
    // Handle material drop from AssetBrowser or MaterialPalette
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            std::string draggedAsset = static_cast<const char*>(payload->Data);
            
            std::filesystem::path assetPath(draggedAsset);
            std::string extension = assetPath.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            if (extension == ".json") {
                std::string filename = assetPath.stem().string();
                std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                
                if (filename.find("material") != std::string::npos) {
                    BGE_LOG_INFO("SculptingPanel", "Material " + draggedAsset + " dropped for sculpting");
                    // TODO: Set brush material from asset
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

} // namespace BGE