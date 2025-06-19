#include "SceneViewPanel.h"
#include "../../Services.h"
#include "../../ServiceLocator.h"
#include "../../../Renderer/Renderer.h"
#include "../../Components.h"
#include "../../Entity.h"  // Need full definition before EntityManager.h
#include "../../ECS/EntityManager.h"
#include "../../ECS/EntityQuery.h"
#include "../../Math/Matrix4.h"
#include "../../Math/Quaternion.h"
#include "../../Logger.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <unordered_map>
#include <GLFW/glfw3.h>

namespace BGE {

SceneViewPanel::SceneViewPanel(const std::string& name, SimulationWorld* world)
    : Panel(name, PanelDockPosition::Center)
    , m_world(world)
    , m_transformGizmo(std::make_unique<TransformGizmo>())
    , m_gizmo2D(std::make_unique<Gizmo2D>()) {
}

SceneViewPanel::~SceneViewPanel() {
    UnregisterEventListeners();
}

void SceneViewPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RegisterEventListeners();
    
    // Center camera on world
    auto world = Services::GetWorld();
    if (world) {
        float centerX = world->GetWidth() / 2.0f;
        float centerY = world->GetHeight() / 2.0f;
        m_editorCameraPos = Vector2(centerX, centerY);
        BGE_LOG_INFO("SceneViewPanel", "Initialized camera at world center: (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");
    }
    
    // Initialize 3D gizmo
    m_transformGizmo->SetMode(m_gizmoMode);
    m_transformGizmo->SetSpace(m_gizmoSpace);
    m_transformGizmo->SetSnapping(false, 1.0f, 15.0f, 0.1f);
    m_transformGizmo->SetCallback([this](const Vector3& pos, const Quaternion& rot, const Vector3& scale) {
        OnGizmoTransformChanged(pos, rot, scale);
    });
    
    // Initialize 2D gizmo
    m_gizmo2D->SetCallback([this](const Vector2& pos, float rotation, const Vector2& scale) {
        OnGizmo2DTransformChanged(pos, rotation, scale);
    });
    
    // Set world-to-screen transformation
    m_gizmo2D->SetWorldToScreenFunc([this](const Vector2& worldPos) -> Vector2 {
        ImVec2 screenPos = WorldToScreen(worldPos);
        return Vector2(screenPos.x, screenPos.y);
    });
    
    // Set screen-to-world transformation
    m_gizmo2D->SetScreenToWorldFunc([this](const Vector2& screenPos) -> Vector2 {
        Vector2 worldPos = ScreenToWorld(ImVec2(screenPos.x, screenPos.y));
        return worldPos;
    });
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
    
    // Gizmo mode buttons
    if (ImGui::Button("Translate") || (ImGui::IsKeyPressed(ImGuiKey_W) && m_isFocused)) {
        m_gizmoMode = GizmoMode::Translate;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate Mode (W)");
    
    ImGui::SameLine();
    if (ImGui::Button("Rotate") || (ImGui::IsKeyPressed(ImGuiKey_E) && m_isFocused)) {
        m_gizmoMode = GizmoMode::Rotate;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate Mode (E)");
    
    ImGui::SameLine();
    if (ImGui::Button("Scale") || (ImGui::IsKeyPressed(ImGuiKey_R) && m_isFocused)) {
        m_gizmoMode = GizmoMode::Scale;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale Mode (R)");
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Gizmo space toggle
    if (ImGui::Button(m_gizmoSpace == GizmoSpace::World ? "World" : "Local")) {
        m_gizmoSpace = (m_gizmoSpace == GizmoSpace::World) ? GizmoSpace::Local : GizmoSpace::World;
        m_transformGizmo->SetSpace(m_gizmoSpace);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle World/Local Space");
    
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
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
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Store viewport info
    m_viewportWidth = contentRegion.x;
    m_viewportHeight = contentRegion.y;
    m_viewportScreenPos = cursorPos;
    
    auto world = Services::GetWorld();
    
    if (world && m_viewportWidth > 0 && m_viewportHeight > 0) {
        // Create invisible button for input handling
        ImGui::SetCursorScreenPos(cursorPos);
        ImGui::InvisibleButton("SceneViewport", contentRegion);
        bool isViewportHovered = ImGui::IsItemHovered();
        
        // Draw the scene using ImGui draw list (more reliable than OpenGL direct rendering)
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Background
        drawList->AddRectFilled(cursorPos, 
                               ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), 
                               IM_COL32(25, 25, 30, 255));
        
        // Set clip rect to viewport
        drawList->PushClipRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), true);
        
        // Draw world simulation pixels
        RenderWorldPixels(drawList, world.get());
        
        // Draw grid if enabled
        if (m_showGrid) {
            RenderGridOverlay(drawList);
        }
        
        // Draw entities
        RenderEntitiesOverlay(drawList);
        
        // Draw gizmos
        if (m_showGizmos) {
            RenderGizmos();
        }
        
        drawList->PopClipRect();
        
        // Add viewport border
        ImU32 borderColor = m_isFocused ? IM_COL32(100, 150, 255, 150) : IM_COL32(100, 100, 100, 100);
        drawList->AddRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), 
                         borderColor, 0.0f, 0, 1.0f);
        
        // Handle input
        if (isViewportHovered) {
            HandleEditorCameraInput();
            
            // Handle gizmo input for selected entities
            if (m_showGizmos && !m_selectedEntities.empty() && m_primarySelection != INVALID_ENTITY) {
                auto& entityManager = EntityManager::Instance();
                if (entityManager.IsEntityValid(m_primarySelection)) {
                    auto* transform = entityManager.GetComponent<TransformComponent>(m_primarySelection);
                    if (transform) {
                        ImVec2 globalMousePos = ImGui::GetMousePos();
                        Vector2 mousePosVec(globalMousePos.x, globalMousePos.y);
                        bool mouseDown = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                        bool mouseHeld = ImGui::IsMouseDown(ImGuiMouseButton_Left);
                        bool mouseDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f);
                        
                        // Only update gizmo position from transform when not actively dragging
                        if (m_gizmo2D->GetActiveAxis() == Gizmo2DAxis::None) {
                            m_gizmo2D->SetPosition(Vector2(transform->position.x, transform->position.y));
                            m_gizmo2D->SetRotation(transform->rotation);
                            m_gizmo2D->SetScale(Vector2(transform->scale.x, transform->scale.y));
                        }
                        
                        // Handle input
                        bool effectiveDragging = mouseDragging || (mouseHeld && m_gizmo2D->GetActiveAxis() != Gizmo2DAxis::None);
                        bool gizmoHandled = m_gizmo2D->HandleInput(mousePosVec, mouseDown, effectiveDragging, m_editorCameraZoom);
                        
                        if (gizmoHandled) {
                            // Don't process other input if gizmo handled it
                            ImGui::GetIO().WantCaptureMouse = true;
                        }
                    }
                }
            }
        }
    }
}

void SceneViewPanel::HandleEditorCameraInput() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Check for gizmo interaction 
    bool gizmoActive = (m_gizmo2D && m_gizmo2D->GetActiveAxis() != Gizmo2DAxis::None);
    
    // Check if mouse is over gizmo (early detection to prevent entity selection)
    bool gizmoHovered = false;
    if (m_gizmo2D && m_showGizmos && !m_selectedEntities.empty()) {
        Vector2 mousePosVec(mousePos.x, mousePos.y);
        gizmoHovered = m_gizmo2D->IsMouseOverGizmo(mousePosVec, m_editorCameraZoom);
    }
    
    // Pan with middle mouse button (only if gizmo isn't active)
    if (!gizmoActive && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        m_panning = true;
        m_lastMousePos = Vector2(mousePos.x, mousePos.y);
        BGE_LOG_DEBUG("SceneViewPanel", "Started panning at (" + std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y) + ")");
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        if (m_panning) {
            BGE_LOG_DEBUG("SceneViewPanel", "Stopped panning");
        }
        m_panning = false;
    }
    
    if (!gizmoActive && m_panning && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        Vector2 currentMousePos(mousePos.x, mousePos.y);
        Vector2 delta = currentMousePos - m_lastMousePos;
        
        // Convert screen delta to world delta (accounting for zoom)
        float worldDeltaX = delta.x / m_editorCameraZoom;
        float worldDeltaY = delta.y / m_editorCameraZoom;
        
        BGE_LOG_DEBUG("SceneViewPanel", "Panning delta: (" + std::to_string(delta.x) + ", " + std::to_string(delta.y) + ") -> world: (" + std::to_string(worldDeltaX) + ", " + std::to_string(worldDeltaY) + ")");
        
        PanCamera(worldDeltaX, worldDeltaY);
        m_lastMousePos = currentMousePos;
    }
    
    // Handle mouse clicks for entity selection
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Only handle selection if gizmo isn't being used
        if (!gizmoActive && !gizmoHovered && !m_panning) {
            HandleEntitySelection(mousePos);
        }
    }
    
    // Handle keyboard shortcuts for gizmo modes
    if (m_showGizmos && m_isFocused) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) {
            m_gizmoMode = GizmoMode::Translate;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E)) {
            m_gizmoMode = GizmoMode::Rotate;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            m_gizmoMode = GizmoMode::Scale;
        }
    }
    
    // Zoom with mouse wheel
    if (io.MouseWheel != 0.0f) {
        // Get world position under mouse before zoom
        Vector2 worldPosBeforeZoom = ScreenToWorld(mousePos);
        
        float zoomDelta = io.MouseWheel * 0.1f;
        ZoomCamera(zoomDelta);
        
        // Get world position under mouse after zoom
        Vector2 worldPosAfterZoom = ScreenToWorld(mousePos);
        
        // Adjust camera position to keep the same world point under the mouse
        Vector2 delta = worldPosBeforeZoom - worldPosAfterZoom;
        m_editorCameraPos.x += delta.x;
        m_editorCameraPos.y += delta.y;
    }
}

void SceneViewPanel::HandleEntitySelection(ImVec2 mousePos) {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_panning) {
        // Convert screen position to world position
        Vector2 worldPos = ScreenToWorld(mousePos);
        
        // Find entity at this position
        EntityID entityId = GetEntityAtPosition(worldPos);
        
        if (entityId != INVALID_ENTITY) {
            bool ctrlHeld = ImGui::GetIO().KeyCtrl;
            SelectEntity(entityId, ctrlHeld);
        } else if (!ImGui::GetIO().KeyCtrl) {
            // Clear selection if clicking empty space without Ctrl
            m_selectedEntities.clear();
            m_primarySelection = INVALID_ENTITY;
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

void SceneViewPanel::RenderWorldPixels(ImDrawList* drawList, SimulationWorld* world) {
    // Get visible world bounds
    Vector2 topLeft = ScreenToWorld(m_viewportScreenPos);
    Vector2 bottomRight = ScreenToWorld(ImVec2(m_viewportScreenPos.x + m_viewportWidth, 
                                               m_viewportScreenPos.y + m_viewportHeight));
    
    // Clamp to world bounds
    int startX = std::max(0, static_cast<int>(topLeft.x));
    int startY = std::max(0, static_cast<int>(topLeft.y));
    int endX = std::min(static_cast<int>(world->GetWidth()), static_cast<int>(bottomRight.x) + 1);
    int endY = std::min(static_cast<int>(world->GetHeight()), static_cast<int>(bottomRight.y) + 1);
    
    // Draw world background first
    ImVec2 worldTopLeft = WorldToScreen(Vector2(0, 0));
    ImVec2 worldBottomRight = WorldToScreen(Vector2(static_cast<float>(world->GetWidth()), 
                                                    static_cast<float>(world->GetHeight())));
    drawList->AddRectFilled(worldTopLeft, worldBottomRight, IM_COL32(20, 20, 25, 255));
    
    // Get pixel data
    const uint8_t* pixelData = world->GetPixelData();
    if (!pixelData) return;
    
    // Calculate how many pixels to render based on zoom
    float pixelsPerScreenPixel = 1.0f / m_editorCameraZoom;
    
    // When zoomed out, we need to render larger blocks that represent multiple pixels
    float blockSize = std::max(1.0f, m_editorCameraZoom);
    int step = std::max(1, static_cast<int>(pixelsPerScreenPixel));
    
    // For very zoomed out views, use larger steps
    if (m_editorCameraZoom < 0.125f) {
        step = 16;
        blockSize = step * m_editorCameraZoom;
    } else if (m_editorCameraZoom < 0.25f) {
        step = 8;
        blockSize = step * m_editorCameraZoom;
    } else if (m_editorCameraZoom < 0.5f) {
        step = 4;
        blockSize = step * m_editorCameraZoom;
    }
    
    // Limit pixels for performance
    const int maxPixelsPerFrame = 50000;
    int pixelsDrawn = 0;
    
    // Render pixels with proper coordinate system (Y=0 at bottom in OpenGL)
    for (int y = startY; y < endY; y += step) {
        for (int x = startX; x < endX; x += step) {
            // In the pixel data, Y=0 is at the top, but in world coordinates Y=0 is at bottom
            // So we need to flip Y when reading from pixel data
            int flippedY = world->GetHeight() - 1 - y;
            uint32_t pixelIndex = (flippedY * world->GetWidth() + x) * 4;
            
            uint8_t r = pixelData[pixelIndex + 0];
            uint8_t g = pixelData[pixelIndex + 1];
            uint8_t b = pixelData[pixelIndex + 2];
            uint8_t a = pixelData[pixelIndex + 3];
            
            // Skip empty pixels
            if (a == 0) continue;
            
            // When sampling, find the most common non-empty color in the block
            if (step > 1) {
                // Sample a few pixels in the block to get a representative color
                int sampleCount = 0;
                int totalR = 0, totalG = 0, totalB = 0, totalA = 0;
                
                for (int sy = 0; sy < step && (y + sy) < static_cast<int>(world->GetHeight()); sy += step/2 + 1) {
                    for (int sx = 0; sx < step && (x + sx) < static_cast<int>(world->GetWidth()); sx += step/2 + 1) {
                        int sampleY = world->GetHeight() - 1 - (y + sy);
                        uint32_t sampleIndex = (sampleY * world->GetWidth() + (x + sx)) * 4;
                        
                        uint8_t sa = pixelData[sampleIndex + 3];
                        if (sa > 0) {
                            totalR += pixelData[sampleIndex + 0];
                            totalG += pixelData[sampleIndex + 1];
                            totalB += pixelData[sampleIndex + 2];
                            totalA += sa;
                            sampleCount++;
                        }
                    }
                }
                
                if (sampleCount > 0) {
                    r = static_cast<uint8_t>(totalR / sampleCount);
                    g = static_cast<uint8_t>(totalG / sampleCount);
                    b = static_cast<uint8_t>(totalB / sampleCount);
                    a = static_cast<uint8_t>(totalA / sampleCount);
                } else {
                    continue; // Skip if no valid samples
                }
            }
            
            ImU32 color = IM_COL32(r, g, b, a);
            ImVec2 screenPos = WorldToScreen(Vector2(static_cast<float>(x), static_cast<float>(y)));
            
            // Draw the block
            drawList->AddRectFilled(screenPos, 
                                   ImVec2(screenPos.x + blockSize, screenPos.y + blockSize), 
                                   color);
            
            if (++pixelsDrawn >= maxPixelsPerFrame) {
                return; // Stop drawing to maintain performance
            }
        }
    }
}

void SceneViewPanel::RenderGridOverlay(ImDrawList* drawList) {
    auto world = Services::GetWorld();
    if (!world) return;
    
    float worldSize = static_cast<float>(world->GetWidth());
    float gridSize = 64.0f; // Grid size in world units
    
    // Get visible bounds
    Vector2 topLeft = ScreenToWorld(m_viewportScreenPos);
    Vector2 bottomRight = ScreenToWorld(ImVec2(m_viewportScreenPos.x + m_viewportWidth, 
                                               m_viewportScreenPos.y + m_viewportHeight));
    
    // Calculate grid range
    int startX = static_cast<int>(topLeft.x / gridSize) * static_cast<int>(gridSize);
    int endX = static_cast<int>(bottomRight.x / gridSize + 1) * static_cast<int>(gridSize);
    int startY = static_cast<int>(topLeft.y / gridSize) * static_cast<int>(gridSize);
    int endY = static_cast<int>(bottomRight.y / gridSize + 1) * static_cast<int>(gridSize);
    
    // Clamp to world bounds
    startX = std::max(0, startX);
    endX = std::min(static_cast<int>(worldSize), endX);
    startY = std::max(0, startY);
    endY = std::min(static_cast<int>(worldSize), endY);
    
    // Draw grid lines
    for (float x = static_cast<float>(startX); x <= endX; x += gridSize) {
        ImVec2 start = WorldToScreen(Vector2(x, static_cast<float>(startY)));
        ImVec2 end = WorldToScreen(Vector2(x, static_cast<float>(endY)));
        
        // Major grid lines every 5 cells
        bool isMajor = (static_cast<int>(x) % (static_cast<int>(gridSize) * 5)) == 0;
        ImU32 color = isMajor ? IM_COL32(80, 80, 80, 150) : IM_COL32(60, 60, 60, 100);
        drawList->AddLine(start, end, color);
    }
    
    for (float y = static_cast<float>(startY); y <= endY; y += gridSize) {
        ImVec2 start = WorldToScreen(Vector2(static_cast<float>(startX), y));
        ImVec2 end = WorldToScreen(Vector2(static_cast<float>(endX), y));
        
        bool isMajor = (static_cast<int>(y) % (static_cast<int>(gridSize) * 5)) == 0;
        ImU32 color = isMajor ? IM_COL32(80, 80, 80, 150) : IM_COL32(60, 60, 60, 100);
        drawList->AddLine(start, end, color);
    }
    
    // Draw world bounds
    ImVec2 worldTL = WorldToScreen(Vector2(0, 0));
    ImVec2 worldTR = WorldToScreen(Vector2(worldSize, 0));
    ImVec2 worldBL = WorldToScreen(Vector2(0, worldSize));
    ImVec2 worldBR = WorldToScreen(Vector2(worldSize, worldSize));
    
    ImU32 boundsColor = IM_COL32(150, 150, 150, 255);
    drawList->AddLine(worldTL, worldTR, boundsColor, 2.0f);
    drawList->AddLine(worldTR, worldBR, boundsColor, 2.0f);
    drawList->AddLine(worldBR, worldBL, boundsColor, 2.0f);
    drawList->AddLine(worldBL, worldTL, boundsColor, 2.0f);
    
    // Draw origin marker
    float centerX = worldSize / 2.0f;
    float centerY = worldSize / 2.0f;
    ImVec2 center = WorldToScreen(Vector2(centerX, centerY));
    
    // X axis (red)
    ImVec2 xStart = WorldToScreen(Vector2(centerX - 50, centerY));
    ImVec2 xEnd = WorldToScreen(Vector2(centerX + 50, centerY));
    drawList->AddLine(xStart, xEnd, IM_COL32(255, 100, 100, 200), 3.0f);
    
    // Y axis (green)
    ImVec2 yStart = WorldToScreen(Vector2(centerX, centerY - 50));
    ImVec2 yEnd = WorldToScreen(Vector2(centerX, centerY + 50));
    drawList->AddLine(yStart, yEnd, IM_COL32(100, 255, 100, 200), 3.0f);
}

void SceneViewPanel::RenderEntitiesOverlay(ImDrawList* drawList) {
    auto& entityManager = EntityManager::Instance();
    
    auto allEntities = entityManager.GetAllEntityIDs();
    
    for (EntityID entityId : allEntities) {
        auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
        if (!transform) continue;
        
        ImVec2 screenPos = WorldToScreen(Vector2(transform->position.x, transform->position.y));
        
        // Skip if outside viewport
        if (screenPos.x < m_viewportScreenPos.x - 50 || screenPos.x > m_viewportScreenPos.x + m_viewportWidth + 50 ||
            screenPos.y < m_viewportScreenPos.y - 50 || screenPos.y > m_viewportScreenPos.y + m_viewportHeight + 50) {
            continue;
        }
        
        // Determine entity appearance based on components
        auto* sprite = entityManager.GetComponent<SpriteComponent>(entityId);
        auto* material = entityManager.GetComponent<MaterialComponent>(entityId);
        auto* light = entityManager.GetComponent<LightComponent>(entityId);
        
        bool isSelected = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entityId) != m_selectedEntities.end();
        
        if (material) {
            // Material entity - square
            float size = 16.0f * transform->scale.x * m_editorCameraZoom;
            ImU32 color = IM_COL32(128, 128, 200, 200);
            
            drawList->AddRectFilled(ImVec2(screenPos.x - size/2, screenPos.y - size/2),
                                   ImVec2(screenPos.x + size/2, screenPos.y + size/2), color);
            
            if (isSelected) {
                drawList->AddRect(ImVec2(screenPos.x - size/2 - 2, screenPos.y - size/2 - 2),
                                 ImVec2(screenPos.x + size/2 + 2, screenPos.y + size/2 + 2),
                                 IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
            }
        }
        else if (sprite && sprite->visible) {
            // Sprite entity - rectangle
            float width = sprite->size.x * transform->scale.x * m_editorCameraZoom;
            float height = sprite->size.y * transform->scale.y * m_editorCameraZoom;
            
            ImU32 fillColor = IM_COL32(200, 200, 200, 100);
            ImU32 outlineColor = IM_COL32(150, 150, 150, 255);
            
            drawList->AddRectFilled(ImVec2(screenPos.x - width/2, screenPos.y - height/2),
                                   ImVec2(screenPos.x + width/2, screenPos.y + height/2), fillColor);
            drawList->AddRect(ImVec2(screenPos.x - width/2, screenPos.y - height/2),
                             ImVec2(screenPos.x + width/2, screenPos.y + height/2), outlineColor);
            
            if (isSelected) {
                drawList->AddRect(ImVec2(screenPos.x - width/2 - 2, screenPos.y - height/2 - 2),
                                 ImVec2(screenPos.x + width/2 + 2, screenPos.y + height/2 + 2),
                                 IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
            }
        }
        else if (light) {
            // Light entity
            float size = 20.0f * m_editorCameraZoom;
            ImU32 color = (light->type == LightComponent::Directional) ? 
                         IM_COL32(255, 255, 150, 200) : IM_COL32(255, 200, 100, 200);
            
            if (light->type == LightComponent::Point) {
                drawList->AddCircleFilled(screenPos, size * 0.5f, color);
                drawList->AddCircle(screenPos, size * 0.5f, IM_COL32(255, 255, 255, 255));
                
                if (isSelected) {
                    // Show light range
                    float range = light->range * m_editorCameraZoom;
                    drawList->AddCircle(screenPos, range, IM_COL32(255, 200, 100, 100), 32, 1.0f);
                }
            }
            else {
                // Directional light - sun icon
                drawList->AddCircleFilled(screenPos, size * 0.4f, color);
                for (int i = 0; i < 8; i++) {
                    float angle = i * 3.14159f * 2.0f / 8.0f;
                    ImVec2 inner(screenPos.x + cos(angle) * size * 0.5f, 
                                screenPos.y + sin(angle) * size * 0.5f);
                    ImVec2 outer(screenPos.x + cos(angle) * size * 0.8f, 
                                screenPos.y + sin(angle) * size * 0.8f);
                    drawList->AddLine(inner, outer, color, 2.0f);
                }
            }
            
            if (isSelected) {
                drawList->AddCircle(screenPos, size, IM_COL32(255, 200, 0, 255), 32, 2.0f);
            }
        }
        else {
            // Default entity - diamond
            float size = 10.0f * m_editorCameraZoom;
            ImVec2 points[4] = {
                ImVec2(screenPos.x, screenPos.y - size),
                ImVec2(screenPos.x + size, screenPos.y),
                ImVec2(screenPos.x, screenPos.y + size),
                ImVec2(screenPos.x - size, screenPos.y)
            };
            
            drawList->AddConvexPolyFilled(points, 4, IM_COL32(180, 180, 180, 200));
            drawList->AddPolyline(points, 4, IM_COL32(255, 255, 255, 255), ImDrawFlags_Closed, 1.0f);
            
            if (isSelected) {
                float selSize = size + 3;
                ImVec2 selPoints[4] = {
                    ImVec2(screenPos.x, screenPos.y - selSize),
                    ImVec2(screenPos.x + selSize, screenPos.y),
                    ImVec2(screenPos.x, screenPos.y + selSize),
                    ImVec2(screenPos.x - selSize, screenPos.y)
                };
                drawList->AddPolyline(selPoints, 4, IM_COL32(255, 200, 0, 255), ImDrawFlags_Closed, 2.0f);
            }
        }
        
        // Draw entity name if close enough
        if (m_editorCameraZoom > 0.5f) {
            auto* nameComp = entityManager.GetComponent<NameComponent>(entityId);
            if (nameComp && !nameComp->name.empty()) {
                drawList->AddText(ImVec2(screenPos.x + 5, screenPos.y - 15), 
                                 IM_COL32(255, 255, 255, 200), nameComp->name.c_str());
            }
        }
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
    
    // Render gizmos
    if (m_showGizmos && m_transformGizmo && !m_selectedEntities.empty()) {
        // Get the primary selected entity
        auto& entityManager = EntityManager::Instance();
        if (entityManager.IsEntityValid(m_primarySelection)) {
            auto* transform = entityManager.GetComponent<TransformComponent>(m_primarySelection);
            if (transform) {
                // Create view and projection matrices for 2D rendering
                // For now, we'll use simple orthographic projection
                Matrix4 viewMatrix = Matrix4::Translation(Vector3(-m_editorCameraPos.x, -m_editorCameraPos.y, 0.0f));
                float halfWidth = m_viewportWidth * 0.5f / m_editorCameraZoom;
                float halfHeight = m_viewportHeight * 0.5f / m_editorCameraZoom;
                Matrix4 projMatrix = Matrix4::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, -100.0f, 100.0f);
                
                // Convert 2D transform to 3D for gizmo
                // Gizmo rendering is handled in RenderGizmos()
            }
        }
    }
}

void SceneViewPanel::RenderSelectionOutlines() {
    if (m_selectedEntities.empty()) return;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    for (EntityID entityId : m_selectedEntities) {
        auto& entityManager = EntityManager::Instance();
        if (!entityManager.IsEntityValid(entityId)) continue;
        
        auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
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
    
    for (EntityID entityId : entityManager.GetAllEntityIDs()) {
        auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
        if (!transform) continue;
        
        ImVec2 screenPos = WorldToScreen(Vector2(transform->position.x, transform->position.y));
        
        // Determine icon text based on components and name
        const char* icon = "[?]"; // Default unknown
        ImU32 iconColor = IM_COL32(180, 180, 180, 255); // Default gray
        
        auto* nameComponent = entityManager.GetComponent<NameComponent>(entityId);
        if (nameComponent) {
            std::string name = nameComponent->name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            
            // Check entity type by name
            if (name.find("camera") != std::string::npos) {
                icon = "[C]"; // Camera
                iconColor = IM_COL32(100, 200, 255, 255); // Light blue
            } else if (name.find("light") != std::string::npos) {
                icon = "[L]"; // Light
                iconColor = IM_COL32(255, 255, 100, 255); // Yellow
            } else if (name.find("player") != std::string::npos) {
                icon = "[P]"; // Player
                iconColor = IM_COL32(100, 255, 100, 255); // Green
            } else if (entityManager.HasComponent<SpriteComponent>(entityId)) {
                icon = "[S]"; // Sprite
                iconColor = IM_COL32(255, 150, 255, 255); // Pink
            } else if (entityManager.HasComponent<MaterialComponent>(entityId)) {
                icon = "[M]"; // Material
                iconColor = IM_COL32(255, 180, 100, 255); // Orange
            } else if (entityManager.HasComponent<VelocityComponent>(entityId)) {
                icon = "[V]"; // Velocity/Moving
                iconColor = IM_COL32(150, 255, 150, 255); // Light green
            } else {
                icon = "[E]"; // Generic Entity
                iconColor = IM_COL32(200, 200, 200, 255); // Light gray
            }
        }
        
        drawList->AddText(screenPos, iconColor, icon);
    }
}

void SceneViewPanel::RenderDebugShapes() {
    // TODO: Render collision boundaries, paths, etc.
    // This would depend on what debug information we want to show
}

void SceneViewPanel::RenderEntities() {
    // This method is no longer used - entities are rendered in RenderEntitiesOverlay
    // Kept for compatibility
}


void SceneViewPanel::UpdateEditorCamera() {
    // Update the renderer's camera to match editor camera
    // This will need to be implemented when we add proper scene rendering
}

void SceneViewPanel::PanCamera(float deltaX, float deltaY) {
    // Pan the camera through the world
    // The camera position represents what we're looking at in world space
    // Invert the delta to make dragging feel natural (drag world, not camera)
    m_editorCameraPos.x -= deltaX;
    m_editorCameraPos.y -= deltaY;
}

void SceneViewPanel::ZoomCamera(float zoomDelta) {
    m_editorCameraZoom += zoomDelta;
    m_editorCameraZoom = std::max(0.1f, std::min(5.0f, m_editorCameraZoom));
}

void SceneViewPanel::ResetCamera() {
    // Center camera on the world
    auto world = Services::GetWorld();
    if (world) {
        float centerX = world->GetWidth() / 2.0f;
        float centerY = world->GetHeight() / 2.0f;
        m_editorCameraPos = Vector2(centerX, centerY);
        BGE_LOG_INFO("SceneViewPanel", "Reset camera to world center: (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");
    } else {
        // Default to 512x512 if world not available
        m_editorCameraPos = Vector2(256.0f, 256.0f);
    }
    m_editorCameraZoom = 1.0f;
}

EntityID SceneViewPanel::GetEntityAtPosition(Vector2 worldPos) {
    auto& entityManager = EntityManager::Instance();
    
    // Check entities in reverse order (top to bottom) for proper layering
    EntityID selectedEntity = INVALID_ENTITY;
    float minDistance = FLT_MAX;
    
    for (EntityID entityId : entityManager.GetAllEntityIDs()) {
        auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
        if (!transform) continue;
        
        Vector2 entityPos(transform->position.x, transform->position.y);
        
        // Check based on entity type for more accurate selection
        bool isHit = false;
        float hitDistance = FLT_MAX;
        
        auto* sprite = entityManager.GetComponent<SpriteComponent>(entityId);
        auto* material = entityManager.GetComponent<MaterialComponent>(entityId);
        auto* light = entityManager.GetComponent<LightComponent>(entityId);
        
        if (material) {
            // Material entities - check square bounds
            float size = 16.0f * transform->scale.x;
            float halfSize = size / 2.0f;
            
            if (worldPos.x >= entityPos.x - halfSize && worldPos.x <= entityPos.x + halfSize &&
                worldPos.y >= entityPos.y - halfSize && worldPos.y <= entityPos.y + halfSize) {
                isHit = true;
                hitDistance = 0.0f; // Inside bounds
            }
        }
        else if (sprite && sprite->visible) {
            // Sprite entities - check rectangle bounds
            float halfWidth = (sprite->size.x * transform->scale.x) / 2.0f;
            float halfHeight = (sprite->size.y * transform->scale.y) / 2.0f;
            
            if (worldPos.x >= entityPos.x - halfWidth && worldPos.x <= entityPos.x + halfWidth &&
                worldPos.y >= entityPos.y - halfHeight && worldPos.y <= entityPos.y + halfHeight) {
                isHit = true;
                hitDistance = 0.0f; // Inside bounds
            }
        }
        else if (light) {
            // Light entities - check circle bounds
            float radius = (light->type == LightComponent::Point) ? 10.0f : 20.0f;
            float distance = sqrtf((worldPos.x - entityPos.x) * (worldPos.x - entityPos.x) + 
                                  (worldPos.y - entityPos.y) * (worldPos.y - entityPos.y));
            if (distance <= radius) {
                isHit = true;
                hitDistance = distance;
            }
        }
        else {
            // Default entities - check distance to center
            float distance = sqrtf((worldPos.x - entityPos.x) * (worldPos.x - entityPos.x) + 
                                  (worldPos.y - entityPos.y) * (worldPos.y - entityPos.y));
            if (distance <= 15.0f) { // 15 pixel radius for default entities
                isHit = true;
                hitDistance = distance;
            }
        }
        
        // Update selection if this entity is closer
        if (isHit && hitDistance < minDistance) {
            minDistance = hitDistance;
            selectedEntity = entityId;
        }
    }
    
    return selectedEntity;
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
    // Get viewport position in screen space
    ImVec2 viewportPos = m_viewportScreenPos;
    
    // Convert screen coordinates to viewport-relative coordinates
    float viewportX = screenPos.x - viewportPos.x;
    float viewportY = screenPos.y - viewportPos.y;
    
    // Convert viewport coordinates to normalized coordinates (0 to 1)
    float normalizedX = viewportX / m_viewportWidth;
    float normalizedY = viewportY / m_viewportHeight;
    
    // Apply camera transform to get world coordinates
    // The camera shows a region of the world based on zoom
    float visibleWidth = m_viewportWidth / m_editorCameraZoom;
    float visibleHeight = m_viewportHeight / m_editorCameraZoom;
    
    // Calculate world position
    float worldX = m_editorCameraPos.x - (visibleWidth / 2.0f) + (normalizedX * visibleWidth);
    float worldY = m_editorCameraPos.y - (visibleHeight / 2.0f) + (normalizedY * visibleHeight);
    
    return Vector2(worldX, worldY);
}

ImVec2 SceneViewPanel::WorldToScreen(Vector2 worldPos) {
    // Get viewport position in screen space
    ImVec2 viewportPos = m_viewportScreenPos;
    
    // Calculate visible world region based on camera
    float visibleWidth = m_viewportWidth / m_editorCameraZoom;
    float visibleHeight = m_viewportHeight / m_editorCameraZoom;
    
    // Calculate world bounds visible in viewport
    float worldLeft = m_editorCameraPos.x - (visibleWidth / 2.0f);
    float worldTop = m_editorCameraPos.y - (visibleHeight / 2.0f);
    
    // Convert world position to normalized viewport coordinates (0 to 1)
    float normalizedX = (worldPos.x - worldLeft) / visibleWidth;
    float normalizedY = (worldPos.y - worldTop) / visibleHeight;
    
    // Convert to viewport pixel coordinates
    float viewportX = normalizedX * m_viewportWidth;
    float viewportY = normalizedY * m_viewportHeight;
    
    // Convert to screen coordinates
    return ImVec2(viewportPos.x + viewportX, viewportPos.y + viewportY);
}

// OnGizmoTransformChanged is defined below

void SceneViewPanel::UpdateEditorCameraMatrices() {
    // For 2D view, we'll use a simple orthographic setup
    // View matrix: translate by negative camera position
    m_viewMatrix = Matrix4();
    m_viewMatrix = Matrix4::Translation(Vector3(-m_editorCameraPos.x, -m_editorCameraPos.y, 0.0f));
    
    // Projection matrix: simple 2D orthographic that maps to screen coordinates
    float halfWidth = m_viewportWidth / (2.0f * m_editorCameraZoom);
    float halfHeight = m_viewportHeight / (2.0f * m_editorCameraZoom);
    m_projMatrix = Matrix4::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, -100.0f, 100.0f);
}

void SceneViewPanel::RenderGizmos() {
    if (m_selectedEntities.empty() || m_primarySelection == INVALID_ENTITY) return;
    
    auto& entityManager = EntityManager::Instance();
    if (!entityManager.IsEntityValid(m_primarySelection)) return;
    
    auto* transform = entityManager.GetComponent<TransformComponent>(m_primarySelection);
    if (!transform) return;
    
    // Convert gizmo mode
    switch (m_gizmoMode) {
        case GizmoMode::Translate:
            m_gizmo2D->SetMode(Gizmo2DMode::Translate);
            break;
        case GizmoMode::Rotate:
            m_gizmo2D->SetMode(Gizmo2DMode::Rotate);
            break;
        case GizmoMode::Scale:
            m_gizmo2D->SetMode(Gizmo2DMode::Scale);
            break;
    }
    
    // Render the 2D gizmo
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    Vector2 worldPos(transform->position.x, transform->position.y);
    ImVec2 screenPos = WorldToScreen(worldPos);
    
    // Render gizmo at screen position
    m_gizmo2D->Render(drawList, Vector2(screenPos.x, screenPos.y), m_editorCameraZoom);
}

void SceneViewPanel::HandleGizmoInput() {
    if (m_selectedEntities.empty() || !m_primarySelection) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    Vector2 mousePosVec(mousePos.x, mousePos.y);
    
    bool mouseDown = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool mouseDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    
    // Handle 2D gizmo input
    bool gizmoHandled = m_gizmo2D->HandleInput(mousePosVec, mouseDown, mouseDragging);
    
    // Don't process other input if gizmo handled it
    if (gizmoHandled) {
        // Prevent entity selection when gizmo is active
    }
}

void SceneViewPanel::OnGizmoTransformChanged(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
    if (!m_primarySelection) return;
    
    auto& entityManager = EntityManager::Instance();
    if (!entityManager.IsEntityValid(m_primarySelection)) return;
    
    auto* transform = entityManager.GetComponent<TransformComponent>(m_primarySelection);
    if (!transform) return;
    
    transform->position = position;
    transform->rotation3D = rotation;
    transform->scale = scale;
    transform->rotation = rotation.ToEuler().z; // Update 2D rotation for compatibility
}

void SceneViewPanel::OnGizmo2DTransformChanged(const Vector2& position, float rotation, const Vector2& scale) {
    if (!m_primarySelection) {
        BGE_LOG_ERROR("SceneViewPanel", "OnGizmo2DTransformChanged: No primary selection");
        return;
    }
    
    auto& entityManager = EntityManager::Instance();
    if (!entityManager.IsEntityValid(m_primarySelection)) {
        BGE_LOG_ERROR("SceneViewPanel", "OnGizmo2DTransformChanged: Invalid entity");
        return;
    }
    
    auto* transform = entityManager.GetComponent<TransformComponent>(m_primarySelection);
    if (!transform) {
        BGE_LOG_ERROR("SceneViewPanel", "OnGizmo2DTransformChanged: No transform component");
        return;
    }
    
    // Debug log
    BGE_LOG_DEBUG("SceneViewPanel", "Updating transform - Pos: (" + std::to_string(position.x) + ", " + 
                  std::to_string(position.y) + "), Rot: " + std::to_string(rotation) + 
                  ", Scale: (" + std::to_string(scale.x) + ", " + std::to_string(scale.y) + ")");
    
    // Update position (keep Z unchanged)
    transform->position.x = position.x;
    transform->position.y = position.y;
    
    // Update rotation
    transform->rotation = rotation;
    transform->rotation3D = Quaternion::FromEuler(0, 0, rotation);
    
    // Update scale (keep Z unchanged)
    transform->scale.x = scale.x;
    transform->scale.y = scale.y;
    
    // Inspector will automatically update when it renders
}

} // namespace BGE