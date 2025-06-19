#pragma once

#include "../Framework/Panel.h"
#include "../../Entity.h"
#include "../../Events.h"
#include "../../EventBus.h"
#include "../../Components.h"
#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"
#include "../../AssetTypes.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <memory>
#include <filesystem>

namespace BGE {

class InspectorPanel : public Panel {
public:
    InspectorPanel(const std::string& name);
    ~InspectorPanel();
    
    void Initialize() override;
    void OnRender() override;
    
private:
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    void OnEntitySelectionChanged(const EntitySelectionChangedEvent& event);
    void OnAssetSelectionChanged(const AssetSelectionChangedEvent& event);
    void OnMaterialHover(const MaterialHoverEvent& event);
    
    // Component rendering
    void RenderEntityInspector();
    void RenderMaterialInspector();
    void RenderAssetInspector();
    void RenderMultiSelectionHeader();
    void RenderSingleEntityHeader(EntityID entityId);
    void RenderComponentList(EntityID entityId);
    void RenderAddComponentButton(EntityID entityId);
    void RenderAddComponentPopup();
    
    // Asset thumbnail methods
    void LoadTexturePreview(const std::string& path);
    void SetCustomThumbnailForAsset(const std::string& assetPath);
    void RemoveCustomThumbnailForAsset(const std::string& assetPath);
    std::string OpenNativeFileDialog();
    
    // Individual component renderers
    void RenderTransformComponent(EntityID entityId, TransformComponent* component);
    void RenderNameComponent(EntityID entityId, NameComponent* component);
    void RenderSpriteComponent(EntityID entityId, SpriteComponent* component);
    void RenderVelocityComponent(EntityID entityId, VelocityComponent* component);
    void RenderHealthComponent(EntityID entityId, HealthComponent* component);
    void RenderMaterialComponent(EntityID entityId, MaterialComponent* component);
    void RenderLightComponent(EntityID entityId, LightComponent* component);
    void RenderRigidbodyComponent(EntityID entityId, RigidbodyComponent* component);
    
    // Component header with context menu
    bool RenderComponentHeader(const char* componentName, bool* removeRequested = nullptr);
    void ShowComponentContextMenu(const char* componentName);
    
    // Property input helpers
    bool InputFloat(const char* label, float* value, float speed = 0.1f);
    bool InputFloat3(const char* label, float* values, float speed = 0.1f);
    bool InputVector2(const char* label, Vector2* value, float speed = 0.1f);
    bool InputVector3(const char* label, Vector3* value, float speed = 0.1f);
    bool InputText(const char* label, std::string* value);
    bool InputBool(const char* label, bool* value);
    bool InputInt(const char* label, int* value);
    bool InputUInt(const char* label, uint32_t* value);
    
    // Multi-selection helpers
    bool IsPropertyConsistent(const std::string& propertyName) const;
    void MarkPropertyInconsistent(const std::string& propertyName);
    void ClearInconsistentProperties();
    
    // Component management
    void RemoveComponentFromEntity(EntityID entityId, const std::string& componentType);
    void AddComponentToEntity(EntityID entityId, const std::string& componentType);
    
    // Component copy/paste
    void CopyComponent(EntityID entityId, const std::string& componentType);
    void PasteComponent(EntityID entityId, const std::string& componentType);
    bool CanPasteComponent(const std::string& componentType) const;
    
    // Selection state
    std::vector<EntityID> m_selectedEntities;
    EntityID m_primarySelection = INVALID_ENTITY;
    
    // UI state
    bool m_showAddComponentPopup = false;
    char m_componentSearchFilter[256] = {0};
    std::string m_contextMenuComponent;
    
    // Multi-selection tracking
    std::unordered_set<std::string> m_inconsistentProperties;
    
    // Event bus for selection synchronization
    EventBus* m_eventBus = nullptr;
    
    // Available component types for Add Component menu
    std::vector<std::string> m_availableComponents;
    
    // Material inspector state
    bool m_materialInspectorMode = false;
    uint32_t m_hoveredMaterialID = 0;
    std::string m_hoveredMaterialName;
    std::string m_hoveredMaterialType;
    std::vector<std::string> m_hoveredMaterialTags;
    
    // Asset inspector state
    bool m_assetInspectorMode = false;
    std::string m_selectedAssetPath;
    AssetType m_selectedAssetType = AssetType::Unknown;
    char m_assetNameBuffer[256] = {0};
    
    // Asset thumbnail management
    std::unordered_map<std::string, uint32_t> m_assetThumbnails; // Per-asset custom thumbnails
    uint32_t m_currentAssetTextureId = 0; // Currently loaded texture for preview
    
    // Component clipboard
    struct ComponentClipboard {
        std::string componentType;
        std::vector<uint8_t> componentData;
        bool hasData = false;
    };
    static ComponentClipboard s_componentClipboard;
};

} // namespace BGE