#include "SceneViewPanel.h"
#include "../../Services.h"
#include "../../ServiceLocator.h"
#include "../../../Renderer/Renderer.h"
#include "../../Components.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace BGE {

SceneViewPanel::SceneViewPanel(const std::string& name, SimulationWorld* world)
    : Panel(name, PanelDockPosition::Center)
    , m_world(world) {
}

SceneViewPanel::~SceneViewPanel() {
    UnregisterEventListeners();
}

void SceneViewPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RegisterEventListeners();
}

void SceneViewPanel::RegisterEventListeners() {
    auto eventBusPtr = ServiceLocator::Instance().GetService<EventBus>();
    m_eventBus = eventBusPtr.get();
    if (m_eventBus) {
        m_eventBus->Subscribe<EntitySelectionChangedEvent>([this](const EntitySelectionChangedEvent& event) {
            OnEntitySelectionChanged(event);
        });
    }
}

void SceneViewPanel::UnregisterEventListeners() {
    // EventBus will handle cleanup when it's destroyed
}

void SceneViewPanel::OnEntitySelectionChanged(const EntitySelectionChangedEvent& event) {
    // Update our selection to match external changes (e.g., from Hierarchy)
    m_selectedEntities = event.selectedEntities;
    m_primarySelection = event.primarySelection;
}

void SceneViewPanel::OnRender() {
    m_isFocused = ImGui::IsWindowFocused();
    m_isHovered = ImGui::IsWindowHovered();
    
    RenderSceneToolbar();
    ImGui::Separator();
    RenderSceneContent();
}

void SceneViewPanel::RenderSceneToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    
    // View mode selector
    static int viewMode = 0; // 0 = Scene, 1 = Wireframe, 2 = Lit
    const char* viewModes[] = { "Scene", "Wireframe", "Lit" };
    ImGui::SetNextItemWidth(80);
    ImGui::Combo("##ViewMode", &viewMode, viewModes, 3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("View Mode");
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Display options
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Icons", &m_showEntityIcons);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_showDebugShapes);
    ImGui::SameLine();
    ImGui::Checkbox("Gizmos", &m_showGizmos);
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Camera controls
    if (ImGui::Button("Reset Cam")) {
        ResetCamera();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset Editor Camera");
    
    ImGui::SameLine();
    ImGui::Text("Zoom: %.1fx", m_editorCameraZoom);
    
    // Selection info
    if (!m_selectedEntities.empty()) {
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        if (m_selectedEntities.size() == 1) {
            ImGui::Text("Selected: Entity %u", m_primarySelection);
        } else {
            ImGui::Text("Selected: %zu entities", m_selectedEntities.size());
        }
    }
    
    ImGui::PopStyleVar(2);
}

void SceneViewPanel::RenderSceneContent() {
    // Get available content region for the scene view
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
        // TODO: Create scene render target (separate from game render target)
        // For now, render a placeholder with debug information
        
        // Create invisible button to capture input
        ImGui::InvisibleButton("SceneViewport", contentRegion);
        
        // Handle editor camera input
        if (ImGui::IsItemHovered()) {
            HandleEditorCameraInput();
            HandleEntitySelection(ImGui::GetMousePos());
        }
        
        // Render background
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 bgColor = IM_COL32(45, 45, 48, 255); // Dark editor background
        drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), bgColor);
        
        // Render grid if enabled
        if (m_showGrid) {
            RenderGrid();
        }
        
        // Render world content with editor camera
        UpdateEditorCamera();
        
        // TODO: Render scene with editor camera instead of game camera
        // For now, show placeholder content
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), 
                         IM_COL32(200, 200, 200, 255), 
                         "Scene View (Editor Camera)");
        
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 30), 
                         IM_COL32(150, 150, 150, 255), 
                         "Pan: Middle Mouse | Zoom: Mouse Wheel");
        
        char camInfo[256];
        sprintf(camInfo, "Camera: (%.1f, %.1f) Zoom: %.2fx", 
                m_editorCameraPos.x, m_editorCameraPos.y, m_editorCameraZoom);
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 50), 
                         IM_COL32(150, 150, 150, 255), 
                         camInfo);
        
        // Render editor overlays
        RenderEditorOverlays();
        
        // Add border to show the scene area
        ImU32 borderColor = m_isFocused ? IM_COL32(100, 150, 255, 150) : IM_COL32(100, 100, 100, 100);
        drawList->AddRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), 
                         borderColor, 0.0f, 0, 1.0f);
    }
}

void SceneViewPanel::HandleEditorCameraInput() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Pan with middle mouse button
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        m_panning = true;
        m_lastMousePos = Vector2(mousePos.x, mousePos.y);
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        m_panning = false;
    }
    
    if (m_panning && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        Vector2 currentMousePos(mousePos.x, mousePos.y);
        Vector2 delta = currentMousePos - m_lastMousePos;
        PanCamera(-delta.x / m_editorCameraZoom, -delta.y / m_editorCameraZoom);
        m_lastMousePos = currentMousePos;
    }
    
    // Zoom with mouse wheel
    if (io.MouseWheel != 0.0f) {
        float zoomDelta = io.MouseWheel * 0.1f;
        ZoomCamera(zoomDelta);
    }
}

void SceneViewPanel::HandleEntitySelection(ImVec2 mousePos) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_panning) {
        // Convert screen position to world position
        Vector2 worldPos = ScreenToWorld(mousePos);
        
        // Find entity at this position
        EntityID entityId = GetEntityAtPosition(worldPos);
        
        if (entityId != INVALID_ENTITY_ID) {
            bool ctrlHeld = ImGui::GetIO().KeyCtrl;
            SelectEntity(entityId, ctrlHeld);
        } else if (!ImGui::GetIO().KeyCtrl) {
            // Clear selection if clicking empty space without Ctrl
            m_selectedEntities.clear();
            m_primarySelection = INVALID_ENTITY_ID;
            BroadcastSelectionChanged();
        }
    }
}

void SceneViewPanel::RenderGrid() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    
    float gridSize = 32.0f * m_editorCameraZoom;
    ImU32 gridColor = IM_COL32(80, 80, 80, 100);
    ImU32 majorGridColor = IM_COL32(120, 120, 120, 150);
    
    // Calculate grid offset based on camera position
    float offsetX = fmod(m_editorCameraPos.x * m_editorCameraZoom, gridSize);
    float offsetY = fmod(m_editorCameraPos.y * m_editorCameraZoom, gridSize);
    
    // Draw vertical lines
    for (float x = offsetX; x < contentRegion.x; x += gridSize) {
        ImU32 color = (fmod(x, gridSize * 5) < 2.0f) ? majorGridColor : gridColor;
        drawList->AddLine(ImVec2(cursorPos.x + x, cursorPos.y), 
                         ImVec2(cursorPos.x + x, cursorPos.y + contentRegion.y), color);
    }
    
    // Draw horizontal lines
    for (float y = offsetY; y < contentRegion.y; y += gridSize) {
        ImU32 color = (fmod(y, gridSize * 5) < 2.0f) ? majorGridColor : gridColor;
        drawList->AddLine(ImVec2(cursorPos.x, cursorPos.y + y), 
                         ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + y), color);
    }
}

void SceneViewPanel::RenderEditorOverlays() {
    if (m_showSelectionOutline) {
        RenderSelectionOutlines();
    }
    
    if (m_showEntityIcons) {
        RenderEntityIcons();
    }
    
    if (m_showDebugShapes) {
        RenderDebugShapes();
    }
}

void SceneViewPanel::RenderSelectionOutlines() {
    if (m_selectedEntities.empty()) return;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    for (EntityID entityId : m_selectedEntities) {
        auto& entityManager = EntityManager::Instance();
        Entity* entity = entityManager.GetEntity(entityId);
        if (!entity) continue;
        
        auto* transform = entity->GetComponent<TransformComponent>();
        if (!transform) continue;
        
        // Convert world position to screen position
        ImVec2 screenPos = WorldToScreen(Vector2(transform->position.x, transform->position.y));
        
        // Draw selection outline
        float size = 20.0f * m_editorCameraZoom;
        ImU32 outlineColor = (entityId == m_primarySelection) ? 
                            IM_COL32(255, 150, 0, 255) :    // Orange for primary
                            IM_COL32(255, 255, 0, 200);     // Yellow for others
        
        drawList->AddRect(ImVec2(screenPos.x - size, screenPos.y - size),
                         ImVec2(screenPos.x + size, screenPos.y + size),
                         outlineColor, 0.0f, 0, 2.0f);
    }
}

void SceneViewPanel::RenderEntityIcons() {
    auto& entityManager = EntityManager::Instance();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    for (const auto& [id, entity] : entityManager.GetAllEntities()) {
        auto* transform = entity->GetComponent<TransformComponent>();
        if (!transform) continue;
        
        ImVec2 screenPos = WorldToScreen(Vector2(transform->position.x, transform->position.y));
        
        // Determine icon based on components
        const char* icon = "📦"; // Default
        if (entity->GetComponent<VelocityComponent>()) icon = "🏃";
        else if (entity->GetComponent<MaterialComponent>()) icon = "🎨";
        else if (entity->GetComponent<SpriteComponent>()) icon = "🖼️";
        
        // Check if entity name suggests a specific icon
        auto* nameComponent = entity->GetComponent<NameComponent>();
        if (nameComponent) {
            std::string name = nameComponent->name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find("camera") != std::string::npos) icon = "📷";
            else if (name.find("light") != std::string::npos) icon = "💡";
            else if (name.find("player") != std::string::npos) icon = "🎮";
        }
        
        drawList->AddText(screenPos, IM_COL32(255, 255, 255, 200), icon);
    }
}

void SceneViewPanel::RenderDebugShapes() {
    // TODO: Render collision boundaries, paths, etc.
    // This would depend on what debug information we want to show
}

void SceneViewPanel::UpdateEditorCamera() {
    // Update the renderer's camera to match editor camera
    // This will need to be implemented when we add proper scene rendering
}

void SceneViewPanel::PanCamera(float deltaX, float deltaY) {
    m_editorCameraPos.x += deltaX;
    m_editorCameraPos.y += deltaY;
}

void SceneViewPanel::ZoomCamera(float zoomDelta) {
    m_editorCameraZoom += zoomDelta;
    m_editorCameraZoom = std::max(0.1f, std::min(5.0f, m_editorCameraZoom));
}

void SceneViewPanel::ResetCamera() {
    m_editorCameraPos = Vector2(0.0f, 0.0f);
    m_editorCameraZoom = 1.0f;
}

EntityID SceneViewPanel::GetEntityAtPosition(Vector2 worldPos) {
    auto& entityManager = EntityManager::Instance();
    
    // Simple distance-based selection for now
    float minDistance = 50.0f; // Selection radius in world units
    EntityID closestEntity = INVALID_ENTITY_ID;
    float closestDistance = minDistance;
    
    for (const auto& [id, entity] : entityManager.GetAllEntities()) {
        auto* transform = entity->GetComponent<TransformComponent>();
        if (!transform) continue;
        
        Vector2 entityPos(transform->position.x, transform->position.y);
        float distance = sqrtf((worldPos.x - entityPos.x) * (worldPos.x - entityPos.x) + 
                              (worldPos.y - entityPos.y) * (worldPos.y - entityPos.y));
        
        if (distance < closestDistance) {
            closestDistance = distance;
            closestEntity = id;
        }
    }
    
    return closestEntity;
}

void SceneViewPanel::SelectEntity(EntityID entityId, bool addToSelection) {
    if (addToSelection) {
        // Add to existing selection
        auto it = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entityId);
        if (it == m_selectedEntities.end()) {
            m_selectedEntities.push_back(entityId);
            m_primarySelection = entityId;
        }
    } else {
        // Replace selection
        m_selectedEntities.clear();
        m_selectedEntities.push_back(entityId);
        m_primarySelection = entityId;
    }
    
    BroadcastSelectionChanged();
}

void SceneViewPanel::BroadcastSelectionChanged() {
    if (!m_eventBus) return;
    
    EntitySelectionChangedEvent event(m_selectedEntities);
    event.primarySelection = m_primarySelection;
    m_eventBus->Publish(event);
}

Vector2 SceneViewPanel::ScreenToWorld(ImVec2 screenPos) {
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Convert screen coordinates to viewport-relative coordinates
    float viewportX = screenPos.x - cursorPos.x;
    float viewportY = screenPos.y - cursorPos.y;
    
    // Apply camera transform
    float worldX = (viewportX / m_editorCameraZoom) + m_editorCameraPos.x;
    float worldY = (viewportY / m_editorCameraZoom) + m_editorCameraPos.y;
    
    return Vector2(worldX, worldY);
}

ImVec2 SceneViewPanel::WorldToScreen(Vector2 worldPos) {
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Apply camera transform
    float viewportX = (worldPos.x - m_editorCameraPos.x) * m_editorCameraZoom;
    float viewportY = (worldPos.y - m_editorCameraPos.y) * m_editorCameraZoom;
    
    // Convert to screen coordinates
    return ImVec2(cursorPos.x + viewportX, cursorPos.y + viewportY);
}

} // namespace BGE