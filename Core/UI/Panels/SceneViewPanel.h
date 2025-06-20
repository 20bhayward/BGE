#pragma once

#include "../Framework/Panel.h"
#include "../../../Simulation/SimulationWorld.h"
#include "../../Entity.h"
#include "../../Events.h"
#include "../../EventBus.h"
#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"
#include "../../Math/Matrix4.h"
#include "../../Math/Ray.h"
#include "../Gizmos/TransformGizmo.h"
#include "../Gizmos/Gizmo2D.h"
#include <vector>
#include <unordered_set>
#include <memory>

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
    void RenderEntities();
    void RenderWorldPixels(ImDrawList* drawList, SimulationWorld* world);
    void RenderGridOverlay(ImDrawList* drawList);
    void RenderEntitiesOverlay(ImDrawList* drawList);
    
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
    
    // Gizmo handling
    void RenderGizmos();
    void HandleGizmoInput();
    void OnGizmoTransformChanged(const Vector3& position, const Quaternion& rotation, const Vector3& scale);
    void OnGizmo2DTransformChanged(const Vector2& position, float rotation, const Vector2& scale);
    void UpdateEditorCameraMatrices();
    SimulationWorld* m_world;
    
    // Viewport info
    float m_viewportX = 0;
    float m_viewportY = 0;
    float m_viewportWidth = 800;
    float m_viewportHeight = 600;
    bool m_isHovered = false;
    bool m_isFocused = false;
    
    // Editor camera state (will be centered on world in Initialize)
    Vector2 m_editorCameraPos{1024.0f, 1024.0f}; // Default to 2048x2048 center
    float m_editorCameraZoom = 1.0f;
    bool m_panning = false;
    Vector2 m_lastMousePos{0.0f, 0.0f};
    
    // Selection state
    std::vector<EntityID> m_selectedEntities;
    EntityID m_primarySelection = INVALID_ENTITY;
    
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
    
    // Transform gizmo
    std::unique_ptr<TransformGizmo> m_transformGizmo;
    std::unique_ptr<Gizmo2D> m_gizmo2D;
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    GizmoSpace m_gizmoSpace = GizmoSpace::World;
    
    // Editor camera matrices for gizmo rendering
    Matrix4 m_viewMatrix;
    Matrix4 m_projMatrix;
    
    // Stored viewport position for gizmo rendering
    ImVec2 m_viewportScreenPos;
    
    // Debug
    mutable int m_debugPrintTimer = 0;
};

} // namespace BGE