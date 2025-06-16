#pragma once

#include "../Framework/Panel.h"
#include "../../../Simulation/SimulationWorld.h"
#include "../../Entity.h"
#include "../../Events.h"
#include "../../EventBus.h"
#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"
#include <vector>
#include <unordered_set>

namespace BGE {

class SceneViewPanel : public Panel {
public:
    SceneViewPanel(const std::string& name, SimulationWorld* world);
    ~SceneViewPanel();
    
    void Initialize() override;
    void OnRender() override;
    
    // Get the viewport bounds for rendering
    void GetViewportBounds(float& x, float& y, float& width, float& height) const {
        x = m_viewportX;
        y = m_viewportY;
        width = m_viewportWidth;
        height = m_viewportHeight;
    }
    
    bool IsHovered() const { return m_isHovered; }
    bool IsFocused() const { return m_isFocused; }
    
    // Editor camera access
    Vector2 GetEditorCameraPosition() const { return m_editorCameraPos; }
    float GetEditorCameraZoom() const { return m_editorCameraZoom; }
    
private:
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    void OnEntitySelectionChanged(const EntitySelectionChangedEvent& event);
    
    // Rendering
    void RenderSceneToolbar();
    void RenderSceneContent();
    void RenderEditorOverlays();
    void RenderSelectionOutlines();
    void RenderEntityIcons();
    void RenderDebugShapes();
    void RenderGrid();
    
    // Editor camera controls
    void HandleEditorCameraInput();
    void HandleEntitySelection(ImVec2 mousePos);
    
    // Camera state
    void UpdateEditorCamera();
    void PanCamera(float deltaX, float deltaY);
    void ZoomCamera(float zoomDelta);
    void ResetCamera();
    
    // Selection helpers
    EntityID GetEntityAtPosition(Vector2 worldPos);
    void SelectEntity(EntityID entityId, bool addToSelection = false);
    void BroadcastSelectionChanged();
    
    // Coordinate conversion
    Vector2 ScreenToWorld(ImVec2 screenPos);
    ImVec2 WorldToScreen(Vector2 worldPos);
    
    SimulationWorld* m_world;
    
    // Viewport info
    float m_viewportX = 0;
    float m_viewportY = 0;
    float m_viewportWidth = 800;
    float m_viewportHeight = 600;
    bool m_isHovered = false;
    bool m_isFocused = false;
    
    // Editor camera state
    Vector2 m_editorCameraPos{0.0f, 0.0f};
    float m_editorCameraZoom = 1.0f;
    bool m_panning = false;
    Vector2 m_lastMousePos{0.0f, 0.0f};
    
    // Selection state
    std::vector<EntityID> m_selectedEntities;
    EntityID m_primarySelection = INVALID_ENTITY_ID;
    
    // Display options
    bool m_showGrid = true;
    bool m_showEntityIcons = true;
    bool m_showDebugShapes = false;
    bool m_showSelectionOutline = true;
    bool m_showGizmos = true;
    
    // Input state
    bool m_mousePressed = false;
    Vector2 m_mousePressPos{0.0f, 0.0f};
    
    // Event bus for selection synchronization
    EventBus* m_eventBus = nullptr;
};

} // namespace BGE