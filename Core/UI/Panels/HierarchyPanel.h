#pragma once

#include "../Framework/Panel.h"
#include "../../../Simulation/SimulationWorld.h"
#include "../../Entity.h"
#include "../../Events.h"
#include "../../EventBus.h"
#include <vector>
#include <unordered_set>
#include <string>

namespace BGE {

class HierarchyPanel : public Panel {
public:
    HierarchyPanel(const std::string& name, SimulationWorld* world);
    ~HierarchyPanel();
    
    void Initialize() override;
    void OnRender() override;
    
private:
    // Core rendering
    void RenderEntityHierarchy();
    void RenderEntityNode(EntityID entityId, bool& nodeOpen);
    void RenderRootEntities();
    
    // Selection management
    void SelectEntity(EntityID entityId, bool ctrlHeld, bool shiftHeld);
    void ClearSelection();
    bool IsEntitySelected(EntityID entityId) const;
    void BroadcastSelectionChanged();
    void OnEntitySelectionChanged(const EntitySelectionChangedEvent& event);
    
    // Entity operations
    void RenameEntity(EntityID entityId, const std::string& newName);
    void DeleteSelectedEntities();
    void DuplicateSelectedEntities();
    void CreateChildEntity(EntityID parentId);
    void CreateEmpty();
    void CreateRigidbodyEntity();
    void CreatePointLightEntity();
    void ReparentEntity(EntityID childId, EntityID newParentId);
    
    // UI helpers
    std::string GetEntityDisplayName(EntityID entityId) const;
    const char* GetEntityIcon(EntityID entityId) const;
    void ShowContextMenu();
    void HandleKeyboardInput();
    void HandleDragAndDrop(EntityID entityId);
    void HandleMaterialDragAndDrop(EntityID entityId);
    void ApplyMaterialToEntity(EntityID entityId, const std::string& materialPath);
    
    // Entity hierarchy queries
    std::vector<EntityID> GetRootEntities() const;
    std::vector<EntityID> GetChildEntities(EntityID parentId) const;
    bool HasChildren(EntityID entityId) const;
    
    // Multi-selection helpers
    void SelectRange(EntityID fromEntity, EntityID toEntity);
    std::vector<EntityID> GetEntitiesBetween(EntityID start, EntityID end) const;
    
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    
    SimulationWorld* m_world;
    
    // Selection state
    std::unordered_set<EntityID> m_selectedEntities;
    EntityID m_primarySelection = INVALID_ENTITY;
    EntityID m_lastClickedEntity = INVALID_ENTITY;
    
    // UI state
    EntityID m_renamingEntity = INVALID_ENTITY;
    char m_renameBuffer[256] = {0};
    bool m_showContextMenu = false;
    EntityID m_contextMenuEntity = INVALID_ENTITY;
    
    // Drag and drop state
    EntityID m_draggedEntity = INVALID_ENTITY;
    bool m_isDragging = false;
    
    // Event bus for selection synchronization
    EventBus* m_eventBus = nullptr;
    
    // Node expansion state
    std::unordered_set<EntityID> m_expandedNodes;
};

} // namespace BGE