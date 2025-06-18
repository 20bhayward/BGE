#include "GameViewportPanel.h"
#include "../../Services.h"
#include "../../Logger.h"
#include "../../../Renderer/Renderer.h"
#include "../../Components.h"
#include "../../Entity.h"
#include "../../ECS/EntityManager.h"
#include "../../ECS/EntityQuery.h"
#include <imgui.h>
#include <algorithm>
#include <filesystem>

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
    
    // Render toolbar at the top of the game panel
    RenderViewportToolbar();
    
    // Main game content takes the remaining space
    RenderGameContent();
    
}

void GameViewportPanel::RenderViewportToolbar() {
    // Compact toolbar with essential controls only
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    
    // Play controls group
    bool isPaused = m_world->IsPaused();
    
    if (isPaused) {
        if (ImGui::Button(" Play ")) {
            m_world->Play();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play (P)");
    } else {
        if (ImGui::Button("Pause")) {
            m_world->Pause();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause (P)");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        m_world->Step();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step (S)");
    
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
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
    const char* toolIcons[] = { "Brush", "Erase", "Sample" };
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
    
    ImGui::PopStyleVar(2);
}

void GameViewportPanel::RenderGameContent() {
    // Get available content region for the game
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Calculate viewport bounds in screen space (absolute coordinates)
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
            
            // Create framebuffer for this panel size
            bool success = renderer->CreateGameFramebuffer(static_cast<int>(m_viewportWidth), 
                                                          static_cast<int>(m_viewportHeight));
            if (!success) {
                BGE_LOG_ERROR("GameViewport", "Failed to create game framebuffer");
                return;
            }
            
            BGE_LOG_INFO("GameViewport", "Created game framebuffer: " + 
                        std::to_string(static_cast<int>(m_viewportWidth)) + "x" + 
                        std::to_string(static_cast<int>(m_viewportHeight)));
        }
        
        // Update MaterialTools viewport to match our panel
        m_tools->SetViewport(static_cast<int>(cursorPos.x), static_cast<int>(cursorPos.y), 
                            static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y));
        
        // Render the game to our texture
        renderer->BeginRenderToTexture();
        renderer->BeginFrame();
        renderer->RenderWorld(world.get());  // Convert shared_ptr to raw pointer
        renderer->RenderParticles();
        renderer->EndFrame();
        renderer->EndRenderToTexture();
        
        // Display the rendered texture in the ImGui panel
        uint32_t textureId = renderer->GetGameTextureId();
        if (textureId != 0) {
            // Note: ImGui expects texture coordinates flipped on Y axis for OpenGL
            ImTextureID imguiTextureId = static_cast<ImTextureID>(static_cast<uintptr_t>(textureId));
            ImGui::Image(imguiTextureId, 
                        contentRegion, 
                        ImVec2(0, 1), ImVec2(1, 0)); // Flip Y coordinate for OpenGL
        } else {
            // Fallback: draw a placeholder
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImU32 bgColor = IM_COL32(50, 50, 50, 255);
            drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), bgColor);
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), IM_COL32(255, 255, 255, 255), "Game Texture Not Available");
        }
        
        // Add a subtle border to show the game area
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 borderColor = m_cameraMode ? IM_COL32(0, 150, 255, 100) : IM_COL32(100, 100, 100, 50);
        drawList->AddRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), borderColor, 0.0f, 0, 1.0f);
    
        // Camera mode indicator in corner
        if (m_cameraMode) {
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), IM_COL32(0, 150, 255, 255), "CAMERA MODE - WASD to move, C to exit");
        } else {
            // Show helpful hint about camera mode
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + contentRegion.y - 25), IM_COL32(150, 150, 150, 150), "Press C for camera mode");
        }
        
        // Check if the image is hovered for input handling
        bool imageHovered = ImGui::IsItemHovered();
        
        // Handle material drag and drop on the scene view (only when playing)
        if (imageHovered && !m_world->IsPaused()) {
            HandleMaterialDragAndDrop(ImGui::GetMousePos(), contentRegion);
        }
        
        // Handle keyboard camera movement when panel is focused (regardless of hover)
        if (m_isFocused || imageHovered) {
            HandleKeyboardInput();
        }
        
        // Handle input when image is hovered
        if (imageHovered) {
            HandleImageInput(cursorPos, contentRegion);
        }
    } else {
        // Fallback: create invisible button when no texture available
        ImGui::InvisibleButton("GameViewport", contentRegion);
        
        if (ImGui::IsItemHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relativeX = mousePos.x - cursorPos.x;
            float relativeY = mousePos.y - cursorPos.y;
            HandleCameraInput(relativeX, relativeY);
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
    
    // Drag and drop target (only when playing)
    if (!m_world->IsPaused() && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_ID")) {
            MaterialID materialId = *(MaterialID*)payload->Data;
            m_tools->GetBrush().SetMaterial(materialId);
        }
        ImGui::EndDragDropTarget();
    }
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
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && ImGui::GetIO().KeyShift) {
        camera->SetPosition(Vector2{0, 0});
        camera->SetZoom(1);
    }
}

void GameViewportPanel::HandleImageInput(ImVec2 cursorPos, ImVec2 contentRegion) {
    if (!m_tools) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    float relativeX = mousePos.x - cursorPos.x;
    float relativeY = mousePos.y - cursorPos.y;
    
    // Ensure mouse is within the image bounds
    if (relativeX < 0 || relativeY < 0 || relativeX >= contentRegion.x || relativeY >= contentRegion.y) {
        return;
    }
    
    // Handle camera controls first
    HandleCameraInput(relativeX, relativeY);
    
    // Convert panel-relative coordinates to absolute screen coordinates
    // MaterialTools expects absolute screen coordinates, not relative ones
    // (mousePos already declared above)
    
    // Then handle material tools if not in camera mode, mouse is within bounds, and simulation is playing
    if (!m_cameraMode && !m_world->IsPaused() && relativeX >= 0 && relativeY >= 0 && relativeX < contentRegion.x && relativeY < contentRegion.y) {
        // Use absolute screen coordinates for material tools (they do their own conversion)
        m_tools->OnMouseMoved(mousePos.x, mousePos.y);
        
        // Handle mouse clicks
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
}

void GameViewportPanel::HandleKeyboardInput() {
    // Handle keyboard shortcuts
    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>('C'))) {
        m_cameraMode = !m_cameraMode;
    }
    
    // Handle keyboard camera movement (arrow keys and WASD) - only when in camera mode
    if (!m_cameraMode) {
        return; // Skip camera movement when not in camera mode
    }
    
    auto renderer = Services::GetRenderer();
    if (renderer && renderer->GetPixelCamera()) {
        auto* camera = renderer->GetPixelCamera();
        Vector2 currentPos = camera->GetPosition();
        
        // Much slower movement speed and frame-rate independent
        float baseSpeed = 2.0f; // Reduced from 5.0f
        float zoomFactor = 1.0f / std::max(0.1f, static_cast<float>(camera->GetZoom())); // Prevent division by zero
        float moveSpeed = baseSpeed * zoomFactor;
        
        // Use a simple frame limiting approach - only move every few frames
        static int frameCounter = 0;
        frameCounter++;
        if (frameCounter % 3 != 0) { // Only move every 3rd frame to slow it down
            return;
        }
        
        bool moved = false;
        Vector2 newPos = currentPos;
        
        if (ImGui::IsKeyDown(static_cast<ImGuiKey>(262)) || ImGui::IsKeyDown(static_cast<ImGuiKey>('A'))) { // 262 = Left Arrow
            newPos.x -= moveSpeed;
            moved = true;
        }
        if (ImGui::IsKeyDown(static_cast<ImGuiKey>(263)) || ImGui::IsKeyDown(static_cast<ImGuiKey>('D'))) { // 263 = Right Arrow
            newPos.x += moveSpeed;
            moved = true;
        }
        if (ImGui::IsKeyDown(static_cast<ImGuiKey>(264)) || ImGui::IsKeyDown(static_cast<ImGuiKey>('W'))) { // 264 = Up Arrow
            newPos.y -= moveSpeed;
            moved = true;
        }
        if (ImGui::IsKeyDown(static_cast<ImGuiKey>(265)) || ImGui::IsKeyDown(static_cast<ImGuiKey>('S'))) { // 265 = Down Arrow
            newPos.y += moveSpeed;
            moved = true;
        }
        
        if (moved) {
            camera->SetPosition(newPos);
        }
    }
}

void GameViewportPanel::HandleMaterialDragAndDrop(ImVec2 mousePos, ImVec2 /*contentRegion*/) {
    // Handle material drop from AssetBrowser
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            std::string draggedAsset = static_cast<const char*>(payload->Data);
            
            // Check if the dragged asset is a material
            std::filesystem::path assetPath(draggedAsset);
            std::string extension = assetPath.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            if (extension == ".json") {
                // Check if it's a material file
                std::string filename = assetPath.stem().string();
                std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                
                if (filename.find("material") != std::string::npos || filename.find("mat") != std::string::npos) {
                    // TODO: In a full implementation, you would:
                    // 1. Convert mouse position to world coordinates
                    // 2. Perform a raycast to find the entity under the mouse
                    // 3. Apply the material to that entity
                    
                    // For now, we'll just log the attempt
                    std::cout << "Material " << draggedAsset << " dropped on Scene View at position (" 
                              << mousePos.x << ", " << mousePos.y << ")" << std::endl;
                    
                    // Simple implementation: Apply to first entity with transform component
                    auto& entityManager = EntityManager::Instance();
                    EntityQuery query(&entityManager);
                    auto firstEntity = query.With<TransformComponent>().First();
                    if (firstEntity.IsValid()) {
                        ApplyMaterialToEntity(firstEntity, draggedAsset);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void GameViewportPanel::ApplyMaterialToEntity(EntityID entityId, const std::string& materialPath) {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
        std::cout << "Failed to apply material: Entity " << entityId.id << " not found" << std::endl;
        return;
    }
    
    // Load material ID from file or assign a default
    uint32_t materialID = 1; // Default material ID
    
    try {
        // Simple material ID extraction - in a real implementation,
        // you'd parse the JSON and extract the material ID
        std::string filename = std::filesystem::path(materialPath).stem().string();
        
        // Generate a simple material ID based on filename hash
        // In a real implementation, this would be loaded from the material system
        std::hash<std::string> hasher;
        materialID = static_cast<uint32_t>(hasher(filename) % 1000) + 1;
    } catch (const std::exception& e) {
        std::cout << "Warning: Could not process material file " << materialPath << ": " << e.what() << std::endl;
    }
    
    // Get or add MaterialComponent
    auto* materialComponent = entityManager.GetComponent<MaterialComponent>(entityId);
    if (!materialComponent) {
        MaterialComponent newComponent;
        newComponent.materialID = materialID;
        entityManager.AddComponent(entityId, std::move(newComponent));
    } else {
        // Apply the new material
        materialComponent->materialID = materialID;
    }
    
    std::cout << "Applied material " << materialPath << " (ID: " << materialID << ") to entity " << entityId.id << " via Scene View" << std::endl;
}

} // namespace BGE