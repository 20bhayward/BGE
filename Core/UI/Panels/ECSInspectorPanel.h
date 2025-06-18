#pragma once

#include "../Framework/Panel.h"
#include "../../ECS/EntityManager.h"
#include "../../ECS/ComponentRegistry.h"
#include "../../Events.h"
#include "../../EventBus.h"
#include "../../Components.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <memory>
#include <functional>

namespace BGE {

// Component editor interface
class IComponentEditor {
public:
    virtual ~IComponentEditor() = default;
    virtual void RenderEditor(EntityID entity, void* component) = 0;
    virtual const char* GetComponentName() const = 0;
};

// Template component editor for type-safe editing
template<typename T>
class ComponentEditor : public IComponentEditor {
public:
    using EditorFunction = std::function<void(EntityID, T&)>;
    
    ComponentEditor(const char* name, EditorFunction editor)
        : m_name(name), m_editor(editor) {}
    
    void RenderEditor(EntityID entity, void* component) override {
        m_editor(entity, *static_cast<T*>(component));
    }
    
    const char* GetComponentName() const override { return m_name; }
    
private:
    const char* m_name;
    EditorFunction m_editor;
};

// Enhanced Inspector Panel for ECS
class ECSInspectorPanel : public Panel {
public:
    ECSInspectorPanel(const std::string& name);
    ~ECSInspectorPanel();
    
    void Initialize() override;
    void OnRender() override;
    
    // Register a component editor
    template<typename T>
    void RegisterComponentEditor(const char* displayName, 
                                typename ComponentEditor<T>::EditorFunction editor) {
        ComponentTypeID typeId = ComponentRegistry::Instance().GetComponentTypeID<T>();
        if (typeId != INVALID_COMPONENT_TYPE) {
            m_componentEditors[typeId] = std::make_unique<ComponentEditor<T>>(displayName, editor);
        }
    }
    
private:
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    void OnEntitySelectionChanged(const EntitySelectionChangedEvent& event);
    
    // Rendering
    void RenderEntityInspector();
    void RenderMultiSelectionHeader();
    void RenderSingleEntityHeader(EntityID entityId);
    void RenderComponentList(EntityID entityId);
    void RenderAddComponentMenu(EntityID entityId);
    void RenderArchetypeInfo(EntityID entityId);
    
    // Component operations
    void RemoveComponent(EntityID entityId, ComponentTypeID typeId);
    bool CanAddComponent(EntityID entityId, ComponentTypeID typeId);
    
    // Built-in component editors
    void RegisterBuiltInEditors();
    void RenderTransformEditor(EntityID entity, TransformComponent& transform);
    void RenderVelocityEditor(EntityID entity, VelocityComponent& velocity);
    void RenderNameEditor(EntityID entity, NameComponent& name);
    void RenderHealthEditor(EntityID entity, HealthComponent& health);
    
    // Helper functions
    std::string GetEntityDisplayName(EntityID entityId) const;
    const char* GetComponentIcon(ComponentTypeID typeId) const;
    
    // State
    std::unordered_set<EntityID> m_selectedEntities;
    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentEditor>> m_componentEditors;
    bool m_showArchetypeInfo = false;
    
    // Add component popup state
    bool m_showAddComponentPopup = false;
    EntityID m_addComponentTarget = INVALID_ENTITY;
    char m_componentSearchBuffer[256] = {0};
    
    // Event listener handles
    uint32_t m_selectionChangedHandle = 0;
};

} // namespace BGE