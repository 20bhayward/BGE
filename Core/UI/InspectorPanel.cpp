#include "InspectorPanel.h"
#include "../Services.h"
#include "../../Core/ServiceLocator.h"
#include "../../Core/Entity.h"
#include <imgui.h>
#include <iostream>
#include <algorithm>

namespace BGE {

InspectorPanel::InspectorPanel(const std::string& name)
    : Panel(name, PanelDockPosition::Right) {
    
    // Initialize available component types
    m_availableComponents = {
        "TransformComponent",
        "NameComponent", 
        "SpriteComponent",
        "VelocityComponent",
        "HealthComponent",
        "MaterialComponent",
        "LightComponent",
        "RigidbodyComponent"
    };
}

InspectorPanel::~InspectorPanel() {
    UnregisterEventListeners();
}

void InspectorPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
    RegisterEventListeners();
}

void InspectorPanel::RegisterEventListeners() {
    auto eventBusPtr = ServiceLocator::Instance().GetService<EventBus>();
    m_eventBus = eventBusPtr.get();
    
    if (m_eventBus) {
        m_eventBus->Subscribe<EntitySelectionChangedEvent>([this](const EntitySelectionChangedEvent& event) {
            OnEntitySelectionChanged(event);
        });
        
        m_eventBus->Subscribe<MaterialHoverEvent>([this](const MaterialHoverEvent& event) {
            OnMaterialHover(event);
        });
    }
}

void InspectorPanel::UnregisterEventListeners() {
    // EventBus will handle cleanup when it's destroyed
}

void InspectorPanel::OnEntitySelectionChanged(const EntitySelectionChangedEvent& event) {
    m_selectedEntities = event.selectedEntities;
    m_primarySelection = event.primarySelection;
    ClearInconsistentProperties();
    
    // Exit material inspector mode when an entity is selected
    if (!m_selectedEntities.empty()) {
        m_materialInspectorMode = false;
    }
}

void InspectorPanel::OnMaterialHover(const MaterialHoverEvent& event) {
    if (event.isHovering && m_selectedEntities.empty()) {
        // Enter material inspector mode only when no entity is selected
        m_materialInspectorMode = true;
        m_hoveredMaterialID = event.materialID;
        m_hoveredMaterialName = event.materialName;
        m_hoveredMaterialType = event.materialType;
        m_hoveredMaterialTags = event.materialTags;
    } else {
        // Exit material inspector mode
        m_materialInspectorMode = false;
    }
}

void InspectorPanel::OnRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
    
    // Choose which inspector mode to render
    if (m_materialInspectorMode) {
        RenderMaterialInspector();
    } else {
        RenderEntityInspector();
    }
    
    // Handle Add Component popup
    if (m_showAddComponentPopup) {
        RenderAddComponentPopup();
    }
    
    ImGui::PopStyleVar(2);
}

void InspectorPanel::RenderMaterialInspector() {
    ImGui::Text("🎨 Material Inspector");
    ImGui::Separator();
    
    // Material header
    ImGui::Text("Name: %s", m_hoveredMaterialName.c_str());
    ImGui::Text("Type: %s", m_hoveredMaterialType.c_str());
    ImGui::Text("ID: %u", m_hoveredMaterialID);
    
    // Material tags
    if (!m_hoveredMaterialTags.empty()) {
        ImGui::Text("Tags:");
        ImGui::Indent();
        for (const auto& tag : m_hoveredMaterialTags) {
            ImGui::BulletText("%s", tag.c_str());
        }
        ImGui::Unindent();
    }
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Hover over materials in Scene View to inspect them.");
}

void InspectorPanel::RenderEntityInspector() {
    if (m_selectedEntities.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entity selected");
        ImGui::TextWrapped("Select an entity in the Hierarchy panel to inspect its components.");
        ImGui::Spacing();
        ImGui::TextWrapped("Hover over materials in the Scene View to inspect material properties.");
        return;
    }
    
    // Multi-selection or single selection header
    if (m_selectedEntities.size() > 1) {
        RenderMultiSelectionHeader();
    } else {
        RenderSingleEntityHeader(m_selectedEntities[0]);
    }
    
    ImGui::Separator();
    
    // Render components for primary selection
    if (m_primarySelection != INVALID_ENTITY_ID) {
        RenderComponentList(m_primarySelection);
        
        ImGui::Spacing();
        RenderAddComponentButton(m_primarySelection);
    }
}

void InspectorPanel::RenderMultiSelectionHeader() {
    ImGui::Text("🔗 Multi-Selection (%zu entities)", m_selectedEntities.size());
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "⚠ Editing affects all selected entities");
    
    if (m_primarySelection != INVALID_ENTITY_ID) {
        auto& entityManager = EntityManager::Instance();
        Entity* entity = entityManager.GetEntity(m_primarySelection);
        if (entity) {
            auto* nameComponent = entity->GetComponent<NameComponent>();
            std::string displayName = nameComponent ? nameComponent->name : ("Entity " + std::to_string(m_primarySelection));
            ImGui::Text("Primary: %s", displayName.c_str());
        }
    }
}

void InspectorPanel::RenderSingleEntityHeader(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    Entity* entity = entityManager.GetEntity(entityId);
    
    if (!entity) {
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "⚠ Invalid entity");
        return;
    }
    
    // Entity name and ID
    auto* nameComponent = entity->GetComponent<NameComponent>();
    std::string displayName = nameComponent ? nameComponent->name : ("Entity " + std::to_string(entityId));
    
    ImGui::Text("📦 %s", displayName.c_str());
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(ID: %u)", entityId);
    
    // Active/enabled toggle (if we add that later)
    // For now, just show that it's active
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "●");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Entity is active");
    }
}

void InspectorPanel::RenderComponentList(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    Entity* entity = entityManager.GetEntity(entityId);
    
    if (!entity) {
        return;
    }
    
    // Render each component type
    auto* transform = entity->GetComponent<TransformComponent>();
    if (transform) {
        RenderTransformComponent(entity, transform);
    }
    
    auto* nameComp = entity->GetComponent<NameComponent>();
    if (nameComp) {
        RenderNameComponent(entity, nameComp);
    }
    
    auto* sprite = entity->GetComponent<SpriteComponent>();
    if (sprite) {
        RenderSpriteComponent(entity, sprite);
    }
    
    auto* velocity = entity->GetComponent<VelocityComponent>();
    if (velocity) {
        RenderVelocityComponent(entity, velocity);
    }
    
    auto* health = entity->GetComponent<HealthComponent>();
    if (health) {
        RenderHealthComponent(entity, health);
    }
    
    auto* material = entity->GetComponent<MaterialComponent>();
    if (material) {
        RenderMaterialComponent(entity, material);
    }
    
    auto* light = entity->GetComponent<LightComponent>();
    if (light) {
        RenderLightComponent(entity, light);
    }
    
    auto* rigidbody = entity->GetComponent<RigidbodyComponent>();
    if (rigidbody) {
        RenderRigidbodyComponent(entity, rigidbody);
    }
}

void InspectorPanel::RenderTransformComponent(Entity* entity, TransformComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Transform", &removeRequested)) {
        return; // Component is collapsed
    }
    
    if (removeRequested) {
        // Can't remove Transform component - it's required
        ImGui::OpenPopup("CannotRemoveTransform");
    }
    
    // Transform properties
    bool changed = false;
    
    changed |= InputVector3("Position", &component->position);
    
    float rotationDegrees = component->rotation * (180.0f / 3.14159f);
    if (InputFloat("Rotation", &rotationDegrees)) {
        component->rotation = rotationDegrees * (3.14159f / 180.0f);
        changed = true;
    }
    
    changed |= InputVector3("Scale", &component->scale);
    
    // Parent info (read-only for now)
    if (component->parent != INVALID_ENTITY_ID) {
        ImGui::Text("Parent: Entity %u", component->parent);
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No parent");
    }
    
    if (!component->children.empty()) {
        ImGui::Text("Children: %zu", component->children.size());
    }
    
    // Apply changes to all selected entities if this is multi-selection
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedTransform = selectedEntity->GetComponent<TransformComponent>();
                if (selectedTransform) {
                    selectedTransform->position = component->position;
                    selectedTransform->rotation = component->rotation;
                    selectedTransform->scale = component->scale;
                }
            }
        }
    }
    
    // Cannot remove transform popup
    if (ImGui::BeginPopupModal("CannotRemoveTransform", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Cannot remove Transform component");
        ImGui::Text("Transform is a required component for all entities.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderNameComponent(Entity* entity, NameComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Name", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "NameComponent");
        return;
    }
    
    bool changed = InputText("Name", &component->name);
    
    // Apply changes to all selected entities if this is multi-selection
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedName = selectedEntity->GetComponent<NameComponent>();
                if (selectedName) {
                    selectedName->name = component->name;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderSpriteComponent(Entity* entity, SpriteComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Sprite", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "SpriteComponent");
        return;
    }
    
    bool changed = false;
    
    changed |= InputText("Texture Path", &component->texturePath);
    changed |= InputVector2("Size", &component->size);
    changed |= InputVector2("UV Offset", &component->uvOffset);
    changed |= InputVector2("UV Scale", &component->uvScale);
    changed |= InputBool("Visible", &component->visible);
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedSprite = selectedEntity->GetComponent<SpriteComponent>();
                if (selectedSprite) {
                    selectedSprite->texturePath = component->texturePath;
                    selectedSprite->size = component->size;
                    selectedSprite->uvOffset = component->uvOffset;
                    selectedSprite->uvScale = component->uvScale;
                    selectedSprite->visible = component->visible;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderVelocityComponent(Entity* entity, VelocityComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Velocity", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "VelocityComponent");
        return;
    }
    
    bool changed = false;
    
    changed |= InputVector3("Velocity", &component->velocity);
    changed |= InputVector3("Acceleration", &component->acceleration);
    changed |= InputFloat("Damping", &component->damping, 0.01f);
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedVelocity = selectedEntity->GetComponent<VelocityComponent>();
                if (selectedVelocity) {
                    selectedVelocity->velocity = component->velocity;
                    selectedVelocity->acceleration = component->acceleration;
                    selectedVelocity->damping = component->damping;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderHealthComponent(Entity* entity, HealthComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Health", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "HealthComponent");
        return;
    }
    
    bool changed = false;
    
    changed |= InputFloat("Max Health", &component->maxHealth);
    changed |= InputFloat("Current Health", &component->currentHealth);
    changed |= InputBool("Invulnerable", &component->invulnerable);
    
    // Health percentage bar
    float healthPercent = component->GetHealthPercentage();
    ImVec4 healthColor = healthPercent > 0.6f ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                        healthPercent > 0.3f ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f) :
                                              ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    
    ImGui::TextColored(healthColor, "Health: %.1f%% %s", 
                      healthPercent * 100.0f, 
                      component->IsAlive() ? "🟢" : "💀");
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedHealth = selectedEntity->GetComponent<HealthComponent>();
                if (selectedHealth) {
                    selectedHealth->maxHealth = component->maxHealth;
                    selectedHealth->currentHealth = component->currentHealth;
                    selectedHealth->invulnerable = component->invulnerable;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderMaterialComponent(Entity* entity, MaterialComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Material", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "MaterialComponent");
        return;
    }
    
    bool changed = false;
    
    changed |= InputUInt("Material ID", &component->materialID);
    changed |= InputFloat("Temperature", &component->temperature);
    changed |= InputFloat("Density", &component->density);
    changed |= InputFloat("Hardness", &component->hardness);
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedMaterial = selectedEntity->GetComponent<MaterialComponent>();
                if (selectedMaterial) {
                    selectedMaterial->materialID = component->materialID;
                    selectedMaterial->temperature = component->temperature;
                    selectedMaterial->density = component->density;
                    selectedMaterial->hardness = component->hardness;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

bool InspectorPanel::RenderComponentHeader(const char* componentName, bool* removeRequested) {
    ImGui::PushID(componentName);
    
    // Component header with collapsible arrow and context menu
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | 
                              ImGuiTreeNodeFlags_FramePadding |
                              ImGuiTreeNodeFlags_AllowItemOverlap;
    
    bool isOpen = ImGui::TreeNodeEx("##component", flags, "> %s", componentName);
    
    // Context menu on header
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        m_contextMenuComponent = componentName;
        ImGui::OpenPopup("ComponentContextMenu");
    }
    
    // Three dots menu button
    ImGui::SameLine(ImGui::GetWindowWidth() - 30);
    if (ImGui::SmallButton("⋯")) {
        m_contextMenuComponent = componentName;
        ImGui::OpenPopup("ComponentContextMenu");
    }
    
    // Context menu popup
    if (ImGui::BeginPopup("ComponentContextMenu")) {
        ImGui::Text("%s", componentName);
        ImGui::Separator();
        
        if (ImGui::MenuItem("Reset to Default")) {
            // TODO: Implement reset functionality
        }
        
        if (ImGui::MenuItem("Copy Component")) {
            // TODO: Implement copy functionality  
        }
        
        if (ImGui::MenuItem("Paste Component Values")) {
            // TODO: Implement paste functionality
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Remove Component")) {
            if (removeRequested) {
                *removeRequested = true;
            }
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    
    if (!isOpen) {
        return false; // Component is collapsed
    }
    
    // Component is expanded, but we need to call TreePop later
    // Return true so the caller knows to render content AND call TreePop
    return true; // Component is expanded
}

void InspectorPanel::RenderAddComponentButton(EntityID /*entityId*/) {
    if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
        m_showAddComponentPopup = true;
        m_componentSearchFilter[0] = '\0';
    }
}

void InspectorPanel::RenderAddComponentPopup() {
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal("Add Component", &m_showAddComponentPopup)) {
        // Search filter
        ImGui::InputTextWithHint("##filter", "Search components...", m_componentSearchFilter, sizeof(m_componentSearchFilter));
        
        ImGui::Separator();
        
        // Component list
        std::string filter = m_componentSearchFilter;
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
        
        for (const auto& componentType : m_availableComponents) {
            std::string lowerType = componentType;
            std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
            
            // Filter check
            if (!filter.empty() && lowerType.find(filter) == std::string::npos) {
                continue;
            }
            
            // Check if entity already has this component
            bool hasComponent = false;
            if (m_primarySelection != INVALID_ENTITY_ID) {
                auto& entityManager = EntityManager::Instance();
                Entity* entity = entityManager.GetEntity(m_primarySelection);
                if (entity) {
                    if (componentType == "TransformComponent") hasComponent = entity->GetComponent<TransformComponent>() != nullptr;
                    else if (componentType == "NameComponent") hasComponent = entity->GetComponent<NameComponent>() != nullptr;
                    else if (componentType == "SpriteComponent") hasComponent = entity->GetComponent<SpriteComponent>() != nullptr;
                    else if (componentType == "VelocityComponent") hasComponent = entity->GetComponent<VelocityComponent>() != nullptr;
                    else if (componentType == "HealthComponent") hasComponent = entity->GetComponent<HealthComponent>() != nullptr;
                    else if (componentType == "MaterialComponent") hasComponent = entity->GetComponent<MaterialComponent>() != nullptr;
                    else if (componentType == "LightComponent") hasComponent = entity->GetComponent<LightComponent>() != nullptr;
                    else if (componentType == "RigidbodyComponent") hasComponent = entity->GetComponent<RigidbodyComponent>() != nullptr;
                }
            }
            
            if (hasComponent) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s (already added)", componentType.c_str());
            } else {
                if (ImGui::Selectable(componentType.c_str())) {
                    AddComponentToEntity(m_primarySelection, componentType);
                    m_showAddComponentPopup = false;
                }
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Cancel")) {
            m_showAddComponentPopup = false;
        }
        
        ImGui::EndPopup();
    }
    
    if (m_showAddComponentPopup) {
        ImGui::OpenPopup("Add Component");
    }
}

void InspectorPanel::AddComponentToEntity(EntityID entityId, const std::string& componentType) {
    auto& entityManager = EntityManager::Instance();
    Entity* entity = entityManager.GetEntity(entityId);
    
    if (!entity) return;
    
    if (componentType == "TransformComponent") {
        if (!entity->GetComponent<TransformComponent>()) {
            entity->AddComponent<TransformComponent>();
        }
    } else if (componentType == "NameComponent") {
        if (!entity->GetComponent<NameComponent>()) {
            auto* comp = entity->AddComponent<NameComponent>();
            comp->name = "New Entity";
        }
    } else if (componentType == "SpriteComponent") {
        if (!entity->GetComponent<SpriteComponent>()) {
            entity->AddComponent<SpriteComponent>();
        }
    } else if (componentType == "VelocityComponent") {
        if (!entity->GetComponent<VelocityComponent>()) {
            entity->AddComponent<VelocityComponent>();
        }
    } else if (componentType == "HealthComponent") {
        if (!entity->GetComponent<HealthComponent>()) {
            entity->AddComponent<HealthComponent>();
        }
    } else if (componentType == "MaterialComponent") {
        if (!entity->GetComponent<MaterialComponent>()) {
            entity->AddComponent<MaterialComponent>();
        }
    } else if (componentType == "LightComponent") {
        if (!entity->GetComponent<LightComponent>()) {
            entity->AddComponent<LightComponent>();
        }
    } else if (componentType == "RigidbodyComponent") {
        if (!entity->GetComponent<RigidbodyComponent>()) {
            entity->AddComponent<RigidbodyComponent>();
        }
    }
    
    std::cout << "Added " << componentType << " to entity " << entityId << std::endl;
}

void InspectorPanel::RemoveComponentFromEntity(EntityID entityId, const std::string& componentType) {
    auto& entityManager = EntityManager::Instance();
    Entity* entity = entityManager.GetEntity(entityId);
    
    if (!entity) return;
    
    if (componentType == "NameComponent") {
        entity->RemoveComponent<NameComponent>();
    } else if (componentType == "SpriteComponent") {
        entity->RemoveComponent<SpriteComponent>();
    } else if (componentType == "VelocityComponent") {
        entity->RemoveComponent<VelocityComponent>();
    } else if (componentType == "HealthComponent") {
        entity->RemoveComponent<HealthComponent>();
    } else if (componentType == "MaterialComponent") {
        entity->RemoveComponent<MaterialComponent>();
    } else if (componentType == "LightComponent") {
        entity->RemoveComponent<LightComponent>();
    } else if (componentType == "RigidbodyComponent") {
        entity->RemoveComponent<RigidbodyComponent>();
    }
    
    std::cout << "Removed " << componentType << " from entity " << entityId << std::endl;
}

// Property input helpers with multi-selection support
bool InspectorPanel::InputFloat(const char* label, float* value, float speed) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    return ImGui::DragFloat(label, value, speed);
}

bool InspectorPanel::InputFloat3(const char* label, float* values, float speed) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    return ImGui::DragFloat3(label, values, speed);
}

bool InspectorPanel::InputVector2(const char* label, Vector2* value, float speed) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    float vals[2] = {value->x, value->y};
    if (ImGui::DragFloat2(label, vals, speed)) {
        value->x = vals[0];
        value->y = vals[1];
        return true;
    }
    return false;
}

bool InspectorPanel::InputVector3(const char* label, Vector3* value, float speed) {
    float vals[3] = {value->x, value->y, value->z};
    if (InputFloat3(label, vals, speed)) {
        value->x = vals[0];
        value->y = vals[1];
        value->z = vals[2];
        return true;
    }
    return false;
}

bool InspectorPanel::InputText(const char* label, std::string* value) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    char buffer[256];
    strncpy(buffer, value->c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        *value = buffer;
        return true;
    }
    return false;
}

bool InspectorPanel::InputBool(const char* label, bool* value) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    return ImGui::Checkbox(label, value);
}

bool InspectorPanel::InputInt(const char* label, int* value) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    return ImGui::DragInt(label, value);
}

bool InspectorPanel::InputUInt(const char* label, uint32_t* value) {
    if (m_selectedEntities.size() > 1 && IsPropertyConsistent(label)) {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "Multiple Values");
        ImGui::SameLine();
        ImGui::Text("%s", label);
        return false;
    }
    
    int intValue = static_cast<int>(*value);
    if (ImGui::DragInt(label, &intValue, 1.0f, 0)) {
        *value = static_cast<uint32_t>(std::max(0, intValue));
        return true;
    }
    return false;
}

bool InspectorPanel::IsPropertyConsistent(const std::string& propertyName) const {
    return m_inconsistentProperties.count(propertyName) > 0;
}

void InspectorPanel::MarkPropertyInconsistent(const std::string& propertyName) {
    m_inconsistentProperties.insert(propertyName);
}

void InspectorPanel::ClearInconsistentProperties() {
    m_inconsistentProperties.clear();
}

void InspectorPanel::RenderLightComponent(Entity* entity, LightComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Light", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "LightComponent");
        return;
    }
    
    bool changed = false;
    
    // Light type enum
    const char* lightTypes[] = {"Directional", "Point", "Spot"};
    int currentType = static_cast<int>(component->type);
    if (ImGui::Combo("Type", &currentType, lightTypes, 3)) {
        component->type = static_cast<LightComponent::Type>(currentType);
        changed = true;
    }
    
    changed |= InputVector3("Color", &component->color);
    changed |= InputFloat("Intensity", &component->intensity);
    changed |= InputFloat("Range", &component->range);
    
    if (component->type == LightComponent::Spot) {
        changed |= InputFloat("Inner Cone", &component->innerCone);
        changed |= InputFloat("Outer Cone", &component->outerCone);
    }
    
    changed |= InputBool("Enabled", &component->enabled);
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedLight = selectedEntity->GetComponent<LightComponent>();
                if (selectedLight) {
                    selectedLight->type = component->type;
                    selectedLight->color = component->color;
                    selectedLight->intensity = component->intensity;
                    selectedLight->range = component->range;
                    selectedLight->innerCone = component->innerCone;
                    selectedLight->outerCone = component->outerCone;
                    selectedLight->enabled = component->enabled;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderRigidbodyComponent(Entity* entity, RigidbodyComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Rigidbody", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entity->GetID(), "RigidbodyComponent");
        return;
    }
    
    bool changed = false;
    
    changed |= InputFloat("Mass", &component->mass);
    changed |= InputVector3("Velocity", &component->velocity);
    changed |= InputVector3("Angular Velocity", &component->angularVelocity);
    changed |= InputFloat("Drag", &component->drag);
    changed |= InputFloat("Angular Drag", &component->angularDrag);
    changed |= InputBool("Use Gravity", &component->useGravity);
    changed |= InputBool("Is Kinematic", &component->isKinematic);
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entity->GetID()) continue;
            
            auto& entityManager = EntityManager::Instance();
            Entity* selectedEntity = entityManager.GetEntity(selectedId);
            if (selectedEntity) {
                auto* selectedRigidbody = selectedEntity->GetComponent<RigidbodyComponent>();
                if (selectedRigidbody) {
                    selectedRigidbody->mass = component->mass;
                    selectedRigidbody->velocity = component->velocity;
                    selectedRigidbody->angularVelocity = component->angularVelocity;
                    selectedRigidbody->drag = component->drag;
                    selectedRigidbody->angularDrag = component->angularDrag;
                    selectedRigidbody->useGravity = component->useGravity;
                    selectedRigidbody->isKinematic = component->isKinematic;
                }
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

} // namespace BGE