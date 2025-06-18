#include "ECSInspectorPanel.h"
#include "../../Logger.h"
#include "../../Math/Math.h"
#include <imgui.h>
#include <algorithm>
#include <sstream>

namespace BGE {

ECSInspectorPanel::ECSInspectorPanel(const std::string& name)
    : Panel(name) {
}

ECSInspectorPanel::~ECSInspectorPanel() {
    UnregisterEventListeners();
}

void ECSInspectorPanel::Initialize() {
    RegisterEventListeners();
    RegisterBuiltInEditors();
}

void ECSInspectorPanel::RegisterEventListeners() {
    EventBus::Instance().Subscribe<EntitySelectionChangedEvent>(
        [this](const EntitySelectionChangedEvent& event) {
            OnEntitySelectionChanged(event);
        });
}

void ECSInspectorPanel::UnregisterEventListeners() {
    // EventBus doesn't have unsubscribe in this implementation
    // Would need to be added or use a different approach
}

void ECSInspectorPanel::OnEntitySelectionChanged(const EntitySelectionChangedEvent& event) {
    m_selectedEntities.clear();
    m_selectedEntities.insert(event.selectedEntities.begin(), event.selectedEntities.end());
}

void ECSInspectorPanel::OnRender() {
    if (!IsVisible()) return;
    
    ImGui::Begin(m_name.c_str(), &m_visible);
    
    RenderEntityInspector();
    
    ImGui::End();
}

void ECSInspectorPanel::RenderEntityInspector() {
    if (m_selectedEntities.empty()) {
        ImGui::TextDisabled("No entity selected");
        return;
    }
    
    if (m_selectedEntities.size() > 1) {
        RenderMultiSelectionHeader();
    } else {
        RenderSingleEntityHeader(*m_selectedEntities.begin());
    }
    
    ImGui::Separator();
    
    // For single selection, show components
    if (m_selectedEntities.size() == 1) {
        EntityID entityId = *m_selectedEntities.begin();
        
        if (m_showArchetypeInfo) {
            RenderArchetypeInfo(entityId);
            ImGui::Separator();
        }
        
        RenderComponentList(entityId);
        ImGui::Spacing();
        RenderAddComponentMenu(entityId);
    }
}

void ECSInspectorPanel::RenderMultiSelectionHeader() {
    ImGui::Text("Selected Entities: %zu", m_selectedEntities.size());
    
    // TODO: Show common components
}

void ECSInspectorPanel::RenderSingleEntityHeader(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    
    // Entity name
    std::string name = GetEntityDisplayName(entityId);
    char nameBuffer[256];
    strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
    
    if (ImGui::InputText("##EntityName", nameBuffer, sizeof(nameBuffer))) {
        entityManager.SetEntityName(entityId, nameBuffer);
    }
    
    ImGui::SameLine();
    
    // Entity info
    ImGui::TextDisabled("ID: %u (Gen: %u)", 
        entityId.GetIndex(), entityId.GetGeneration());
    
    // Debug options
    if (ImGui::Button("Archetype Info")) {
        m_showArchetypeInfo = !m_showArchetypeInfo;
    }
}

void ECSInspectorPanel::RenderComponentList(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    auto& registry = ComponentRegistry::Instance();
    
    // Get entity's archetype to find its components
    const auto& allComponents = registry.GetAllComponents();
    
    for (const auto& [typeId, info] : allComponents) {
        // Check if entity has this component type
        // This is a simplified check - in real implementation, 
        // we'd query the archetype directly
        
        // Try to get component editor
        auto editorIt = m_componentEditors.find(typeId);
        if (editorIt != m_componentEditors.end()) {
            // For now, we'll use the entity wrapper approach
            // In a full implementation, we'd directly query the archetype
            Entity* entity = entityManager.GetEntity(entityId.GetIndex());
            if (entity) {
                // Component header
                bool nodeOpen = ImGui::CollapsingHeader(info.name.c_str(), 
                    ImGuiTreeNodeFlags_DefaultOpen);
                
                if (nodeOpen) {
                    ImGui::PushID(static_cast<int>(typeId));
                    
                    // Remove button
                    if (ImGui::Button("Remove")) {
                        RemoveComponent(entityId, typeId);
                    }
                    
                    ImGui::Separator();
                    
                    // Component editor
                    // Note: This is simplified - in real implementation,
                    // Get component directly from EntityManager
                    if (info.name == "TransformComponent") {
                        if (auto* transform = entityManager.GetComponent<TransformComponent>(entityId)) {
                            editorIt->second->RenderEditor(entityId, transform);
                        }
                    } else if (info.name == "VelocityComponent") {
                        if (auto* velocity = entityManager.GetComponent<VelocityComponent>(entityId)) {
                            editorIt->second->RenderEditor(entityId, velocity);
                        }
                    } else if (info.name == "NameComponent") {
                        if (auto* name = entityManager.GetComponent<NameComponent>(entityId)) {
                            editorIt->second->RenderEditor(entityId, name);
                        }
                    } else if (info.name == "HealthComponent") {
                        if (auto* health = entityManager.GetComponent<HealthComponent>(entityId)) {
                            editorIt->second->RenderEditor(entityId, health);
                        }
                    }
                    
                    ImGui::PopID();
                }
            }
        }
    }
}

void ECSInspectorPanel::RenderAddComponentMenu(EntityID entityId) {
    if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
        m_showAddComponentPopup = true;
        m_addComponentTarget = entityId;
    }
    
    if (m_showAddComponentPopup) {
        ImGui::OpenPopup("AddComponentPopup");
    }
    
    if (ImGui::BeginPopup("AddComponentPopup")) {
        ImGui::Text("Add Component");
        ImGui::Separator();
        
        // Search filter
        ImGui::InputText("Search", m_componentSearchBuffer, sizeof(m_componentSearchBuffer));
        
        auto& registry = ComponentRegistry::Instance();
        const auto& allComponents = registry.GetAllComponents();
        
        for (const auto& [typeId, info] : allComponents) {
            // Filter by search
            if (m_componentSearchBuffer[0] != '\0') {
                std::string search(m_componentSearchBuffer);
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                
                std::string compName = info.name;
                std::transform(compName.begin(), compName.end(), compName.begin(), ::tolower);
                
                if (compName.find(search) == std::string::npos) {
                    continue;
                }
            }
            
            // Check if can add
            if (!CanAddComponent(entityId, typeId)) {
                ImGui::TextDisabled("%s (already has)", info.name.c_str());
                continue;
            }
            
            if (ImGui::Selectable(info.name.c_str())) {
                // Add component based on type
                auto& entityManager = EntityManager::Instance();
                if (info.name == "TransformComponent") {
                    TransformComponent comp;
                    entityManager.AddComponent(m_addComponentTarget, std::move(comp));
                } else if (info.name == "VelocityComponent") {
                    VelocityComponent comp;
                    entityManager.AddComponent(m_addComponentTarget, std::move(comp));
                } else if (info.name == "NameComponent") {
                    NameComponent comp("New Entity");
                    entityManager.AddComponent(m_addComponentTarget, std::move(comp));
                } else if (info.name == "HealthComponent") {
                    HealthComponent comp;
                    entityManager.AddComponent(m_addComponentTarget, std::move(comp));
                }
                
                m_showAddComponentPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::EndPopup();
    }
}

void ECSInspectorPanel::RenderArchetypeInfo(EntityID /*entityId*/) {
    // auto& entityManager = EntityManager::Instance();
    // auto& archetypeManager = entityManager.GetArchetypeManager();
    
    ImGui::Text("Archetype Debug Info:");
    
    // This would show the entity's archetype info
    // For now, just show placeholder
    ImGui::TextDisabled("Archetype Index: TODO");
    ImGui::TextDisabled("Component Mask: TODO");
    ImGui::TextDisabled("Entity Count in Archetype: TODO");
}

void ECSInspectorPanel::RemoveComponent(EntityID /*entityId*/, ComponentTypeID /*typeId*/) {
    // TODO: Implement component removal by type ID
    BGE_LOG_WARNING("ECSInspectorPanel", "RemoveComponent not yet implemented");
}

bool ECSInspectorPanel::CanAddComponent(EntityID /*entityId*/, ComponentTypeID /*typeId*/) {
    // TODO: Check if entity already has component
    return true;
}

std::string ECSInspectorPanel::GetEntityDisplayName(EntityID entityId) const {
    auto& entityManager = EntityManager::Instance();
    std::string name = entityManager.GetEntityName(entityId);
    
    if (name.empty()) {
        std::stringstream ss;
        ss << "Entity_" << entityId.GetIndex();
        return ss.str();
    }
    
    return name;
}

const char* ECSInspectorPanel::GetComponentIcon(ComponentTypeID /*typeId*/) const {
    // TODO: Return appropriate icon for component type
    return nullptr;
}

void ECSInspectorPanel::RegisterBuiltInEditors() {
    // Transform Component
    RegisterComponentEditor<TransformComponent>("Transform",
        [this](EntityID entity, TransformComponent& transform) {
            RenderTransformEditor(entity, transform);
        });
    
    // Velocity Component
    RegisterComponentEditor<VelocityComponent>("Velocity",
        [this](EntityID entity, VelocityComponent& velocity) {
            RenderVelocityEditor(entity, velocity);
        });
    
    // Name Component
    RegisterComponentEditor<NameComponent>("Name",
        [this](EntityID entity, NameComponent& name) {
            RenderNameEditor(entity, name);
        });
    
    // Health Component
    RegisterComponentEditor<HealthComponent>("Health",
        [this](EntityID entity, HealthComponent& health) {
            RenderHealthEditor(entity, health);
        });
}

void ECSInspectorPanel::RenderTransformEditor(EntityID /*entity*/, TransformComponent& transform) {
    // Position
    ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
    
    // Rotation (Euler angles)
    Vector3 euler = transform.GetEulerAngles();
    Vector3 eulerDegrees = euler * (180.0f / Math::PI);
    if (ImGui::DragFloat3("Rotation", &eulerDegrees.x, 1.0f)) {
        Vector3 eulerRadians = eulerDegrees * (Math::PI / 180.0f);
        transform.SetEulerAngles(eulerRadians.x, eulerRadians.y, eulerRadians.z);
    }
    
    // Scale
    ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f);
    
    // Reset buttons
    ImGui::Spacing();
    if (ImGui::Button("Reset Position")) transform.position = Vector3(0, 0, 0);
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation")) transform.SetRotation3D(Quaternion());
    ImGui::SameLine();
    if (ImGui::Button("Reset Scale")) transform.scale = Vector3(1, 1, 1);
}

void ECSInspectorPanel::RenderVelocityEditor(EntityID /*entity*/, VelocityComponent& velocity) {
    ImGui::DragFloat3("Linear Velocity", &velocity.velocity.x, 0.1f);
    ImGui::DragFloat3("Angular Velocity", &velocity.angular.x, 0.1f);
    ImGui::DragFloat3("Acceleration", &velocity.acceleration.x, 0.1f);
    ImGui::SliderFloat("Damping", &velocity.damping, 0.0f, 1.0f);
    
    if (ImGui::Button("Reset Velocity")) {
        velocity.velocity = Vector3(0, 0, 0);
        velocity.angular = Vector3(0, 0, 0);
        velocity.acceleration = Vector3(0, 0, 0);
    }
}

void ECSInspectorPanel::RenderNameEditor(EntityID entity, NameComponent& name) {
    char buffer[256];
    strncpy(buffer, name.name.c_str(), sizeof(buffer) - 1);
    
    if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
        name.name = buffer;
        // Also update entity name in manager
        EntityManager::Instance().SetEntityName(entity, buffer);
    }
}

void ECSInspectorPanel::RenderHealthEditor(EntityID /*entity*/, HealthComponent& health) {
    ImGui::DragFloat("Current Health", &health.currentHealth, 1.0f, 0.0f, health.maxHealth);
    ImGui::DragFloat("Max Health", &health.maxHealth, 1.0f, 0.0f, 10000.0f);
    ImGui::Checkbox("Invulnerable", &health.invulnerable);
    
    float healthPercent = health.maxHealth > 0 ? health.currentHealth / health.maxHealth : 0.0f;
    ImGui::ProgressBar(healthPercent, ImVec2(-1, 0), "Health");
    
    if (ImGui::Button("Heal Full")) {
        health.currentHealth = health.maxHealth;
    }
    ImGui::SameLine();
    if (ImGui::Button("Damage (10)")) {
        health.TakeDamage(10.0f);
    }
}

} // namespace BGE