#pragma once

#include "../Framework/Panel.h"
#include "../../../Simulation/SimulationWorld.h"
#include "../../Entity.h"
#include "../../Events.h"
#include "../../EventBus.h"
#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <deque>
#include <memory>

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
    void CreateDirectionalLightEntity();
    void CreateSpotLightEntity();
    void CreateAreaLightEntity();
    void CreateParticleSystemEntity();
    void CreateTrailRendererEntity();
    void CreatePointLightChild(EntityID parentId);
    void ReparentEntity(EntityID childId, EntityID newParentId);
    
    // UI helpers
    std::string GetEntityDisplayName(EntityID entityId) const;
    const char* GetEntityIcon(EntityID entityId) const;
    ImVec4 GetEntityIconColor(EntityID entityId) const;
    void ShowContextMenu();
    void HandleKeyboardInput();
    void HandleDragAndDrop(EntityID entityId);
    void HandleMaterialDragAndDrop(EntityID entityId);
    void HandleDragDropPayload(EntityID targetEntity, EntityID draggedEntity);
    void ApplyMaterialToEntity(EntityID entityId, const std::string& materialPath);
    void FocusCameraOnEntity(EntityID entityId);
    
    // Entity hierarchy queries
    std::vector<EntityID> GetRootEntities() const;
    std::vector<EntityID> GetChildEntities(EntityID parentId) const;
    bool HasChildren(EntityID entityId) const;
    
    // Multi-selection helpers
    void SelectRange(EntityID fromEntity, EntityID toEntity);
    void SelectChildren(EntityID parentEntity);
    std::vector<EntityID> GetEntitiesBetween(EntityID start, EntityID end) const;
    
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    
    // Keyboard shortcuts
    void HandleKeyboardShortcuts();
    void SelectAll();
    void DeselectAll();
    void CopySelectedEntities();
    void PasteEntities();
    void CutSelectedEntities();
    bool HasClipboardData() const;
    
    // Helper functions
    bool IsChildOf(EntityID potentialChild, EntityID potentialParent) const;
    void CollectVisibleEntitiesInOrder(std::vector<EntityID>& outEntities) const;
    void CollectEntityAndChildrenInOrder(EntityID entity, std::vector<EntityID>& outEntities) const;
    
    // Search and filtering
    void UpdateSearchResults();
    bool MatchesSearchFilter(EntityID entityId) const;
    bool IsVisible(EntityID entityId) const;
    
    // Visibility and locking
    void ToggleVisibility(EntityID entityId);
    void ToggleLock(EntityID entityId);
    bool IsEntityVisible(EntityID entityId) const;
    bool IsEntityLocked(EntityID entityId) const;
    
    // Undo/Redo system
    struct EntityOperation {
        enum Type { Create, Delete, Rename, Reparent, ComponentChange };
        Type type;
        EntityID entityId;
        EntityID parentId;
        std::string oldName;
        std::string newName;
        std::vector<EntityID> affectedEntities;
    };
    
    void RecordOperation(const EntityOperation& op);
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
    // Enhanced drag & drop
    enum class DropPosition { Above, Below, Inside };
    DropPosition GetDropPosition(const ImVec2& mousePos, const ImRect& itemRect) const;
    void DrawDropIndicator(DropPosition pos, const ImRect& itemRect);
    
    // Arrow key navigation
    void NavigateUp();
    void NavigateDown();
    void ExpandCollapseSelected(bool expand);
    EntityID GetNextVisibleEntity(EntityID current, bool forward) const;
    
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
    
    // Clipboard state
    struct EntityClipboardData {
        std::vector<EntityID> entities;
        bool isCut = false;
    };
    static EntityClipboardData s_clipboard;
    
    // Search/filter state
    char m_searchBuffer[256] = {0};
    std::string m_searchQuery;
    std::unordered_set<EntityID> m_searchResults;
    bool m_showOnlySearchResults = false;
    
    // Visibility and lock state
    std::unordered_set<EntityID> m_hiddenEntities;
    std::unordered_set<EntityID> m_lockedEntities;
    
    // Undo/Redo state
    static constexpr size_t MAX_UNDO_HISTORY = 50;
    std::deque<EntityOperation> m_undoHistory;
    std::deque<EntityOperation> m_redoHistory;
    
    // Enhanced drag & drop state
    DropPosition m_currentDropPosition = DropPosition::Inside;
    EntityID m_dropTargetEntity = INVALID_ENTITY;
    ImRect m_dropTargetRect;
    
    // Stats tracking
    struct HierarchyStats {
        size_t totalEntities = 0;
        size_t visibleEntities = 0;
        size_t selectedEntities = 0;
        size_t lockedEntities = 0;
    } m_stats;
    
    // Visual settings
    bool m_showIcons = true;
    bool m_showVisibilityToggles = true;
    bool m_showLockToggles = true;
    float m_indentSize = 16.0f;
    
    // UI Configuration
    float m_clearButtonWidth = 20.0f;
    float m_visibilityButtonWidth = 30.0f;
    float m_lockButtonWidth = 30.0f;
    float m_doubleClickTime = 0.3f;
    float m_dropZoneThreshold = 0.3f;
    Vector3 m_defaultEntityPosition = Vector3(1024.0f, 1024.0f, 0.0f);
    Vector2 m_defaultSpriteSize = Vector2(32.0f, 32.0f);
    
    // Performance optimization
    mutable std::unordered_map<EntityID, std::vector<EntityID>> m_childrenCache;
    mutable bool m_childrenCacheDirty = true;
    mutable bool m_hierarchyDirty = true;
    mutable uint32_t m_lastEntityCount = 0;
    mutable std::vector<EntityID> m_cachedRootEntities;
    mutable std::unordered_map<EntityID, std::string> m_cachedDisplayNames;
    mutable std::unordered_map<EntityID, const char*> m_cachedIcons;
    
    void InvalidateChildrenCache() const { m_childrenCacheDirty = true; }
    void InvalidateHierarchy() const { m_hierarchyDirty = true; }
    void UpdateCaches() const;
    
    // Root entity ordering
    std::vector<EntityID> m_rootEntityOrder;
};

} // namespace BGE