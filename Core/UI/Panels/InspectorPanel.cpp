#include "InspectorPanel.h"
#include "ProjectSettingsPanel.h"
#include "../../Services.h"
#include "../../ServiceLocator.h"
#include "../../Entity.h"
#include "../../ECS/EntityManager.h"
#include "../../ECS/EntityQuery.h"
#include "../../../Renderer/Renderer.h"
#include "../../../AssetPipeline/AssetManager.h"
#include "../../../ThirdParty/stb/stb_image.h"
#include <imgui.h>
#include <iostream>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace BGE {

// Static clipboard definition
InspectorPanel::ComponentClipboard InspectorPanel::s_componentClipboard;

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
        
        m_eventBus->Subscribe<AssetSelectionChangedEvent>([this](const AssetSelectionChangedEvent& event) {
            OnAssetSelectionChanged(event);
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
    
    // Exit material and asset inspector modes when an entity is selected
    if (!m_selectedEntities.empty()) {
        m_materialInspectorMode = false;
        m_assetInspectorMode = false;
    }
}

void InspectorPanel::OnAssetSelectionChanged(const AssetSelectionChangedEvent& event) {
    // Clean up previous texture preview if switching assets
    if (m_currentAssetTextureId != 0) {
        auto renderer = Services::GetRenderer();
        if (renderer) {
            renderer->DeleteTexture(m_currentAssetTextureId);
        }
        m_currentAssetTextureId = 0;
    }
    
    if (!event.selectedAssetPath.empty()) {
        // Clear entity selection when an asset is selected
        m_selectedEntities.clear();
        m_primarySelection = INVALID_ENTITY;
        m_materialInspectorMode = false;
        
        // Enter asset inspector mode
        m_assetInspectorMode = true;
        m_selectedAssetPath = event.selectedAssetPath;
        m_selectedAssetType = event.selectedAssetType;
        
        // Initialize asset name buffer for renaming
        std::filesystem::path assetPath(event.selectedAssetPath);
        std::string assetName = assetPath.stem().string();
        strncpy(m_assetNameBuffer, assetName.c_str(), sizeof(m_assetNameBuffer) - 1);
        m_assetNameBuffer[sizeof(m_assetNameBuffer) - 1] = '\0';
    } else {
        // Asset deselected
        m_assetInspectorMode = false;
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
    if (m_assetInspectorMode) {
        RenderAssetInspector();
    } else if (m_materialInspectorMode) {
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

void InspectorPanel::RenderAssetInspector() {
    if (m_selectedAssetPath.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No asset selected");
        return;
    }
    
    std::filesystem::path assetPath(m_selectedAssetPath);
    std::string assetName = assetPath.stem().string();
    std::string extension = assetPath.extension().string();
    
    // Asset header with icon
    std::string typeIcon = "📄";
    switch (m_selectedAssetType) {
        case AssetType::Texture: typeIcon = "🖼️"; break;
        case AssetType::Material: typeIcon = "🎨"; break;
        case AssetType::Scene: typeIcon = "🌍"; break;
        case AssetType::Audio: typeIcon = "🔊"; break;
        case AssetType::Script: typeIcon = "📜"; break;
        case AssetType::Prefab: typeIcon = "🧩"; break;
        case AssetType::Folder: typeIcon = "📁"; break;
        default: typeIcon = "📄"; break;
    }
    
    ImGui::Text("%s Asset Inspector", typeIcon.c_str());
    ImGui::Separator();
    
    // Asset name (editable)
    ImGui::Text("Name:");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##AssetName", m_assetNameBuffer, sizeof(m_assetNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Handle rename
        std::string newName = m_assetNameBuffer;
        if (!newName.empty() && newName != assetName) {
            std::string newPath = assetPath.parent_path().string() + "/" + newName + extension;
            try {
                std::filesystem::rename(m_selectedAssetPath, newPath);
                m_selectedAssetPath = newPath;
            } catch (const std::exception& e) {
                std::cerr << "Failed to rename asset: " << e.what() << std::endl;
            }
        }
    }
    
    // Asset properties
    ImGui::Spacing();
    ImGui::Text("Type: %s", extension.c_str());
    ImGui::Text("Path: %s", m_selectedAssetPath.c_str());
    
    // File size
    try {
        if (std::filesystem::exists(m_selectedAssetPath)) {
            auto fileSize = std::filesystem::file_size(m_selectedAssetPath);
            if (fileSize < 1024) {
                ImGui::Text("Size: %zu bytes", fileSize);
            } else if (fileSize < 1024 * 1024) {
                ImGui::Text("Size: %.2f KB", fileSize / 1024.0);
            } else {
                ImGui::Text("Size: %.2f MB", fileSize / (1024.0 * 1024.0));
            }
        }
    } catch (const std::exception&) {
        ImGui::Text("Size: Unknown");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Custom thumbnail section
    ImGui::Text("Thumbnail:");
    
    // Use the same hybrid lookup system as AssetBrowser
    uint32_t thumbnailId = 0;
    auto projectSettings = Services::GetProjectSettings();
    
    if (projectSettings) {
        // First try to get asset handle for current asset
        AssetHandle assetHandle;
        auto assetManager = ServiceLocator::Instance().GetService<AssetManager>();
        if (assetManager) {
            assetHandle = assetManager->GetRegistry().GetAssetHandle(m_selectedAssetPath);
        }
        
        // First check for individual asset thumbnail using asset handle
        if (assetHandle.IsValid()) {
            thumbnailId = projectSettings->GetIndividualAssetThumbnail(assetHandle);
        }
        
        // If no thumbnail found by handle, try path-based lookup (for moved/recreated assets)
        if (thumbnailId == 0) {
            thumbnailId = projectSettings->GetIndividualAssetThumbnailByPath(m_selectedAssetPath);
            
            // If found by path but not by handle, restore the mapping
            if (thumbnailId != 0 && assetHandle.IsValid()) {
                projectSettings->RestoreThumbnailFromPath(assetHandle, m_selectedAssetPath);
            }
        }
    }
    
    // If no custom thumbnail, check for texture preview or type-based thumbnail
    if (thumbnailId == 0) {
        if (m_selectedAssetType == AssetType::Texture) {
            // For texture assets, load and show the texture itself
            if (m_currentAssetTextureId == 0) {
                LoadTexturePreview(m_selectedAssetPath);
            }
            thumbnailId = m_currentAssetTextureId;
        } else {
            // Use default thumbnail from ProjectSettings
            if (projectSettings) {
                thumbnailId = projectSettings->GetAssetTypeThumbnailTexture(m_selectedAssetType);
            }
        }
    }
    
    // Display thumbnail
    if (thumbnailId != 0) {
        ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(thumbnailId)), ImVec2(128, 128));
    } else {
        ImGui::Button("No Thumbnail", ImVec2(128, 128));
    }
    
    ImGui::SameLine();
    ImGui::BeginGroup();
    
    if (ImGui::Button("Set Custom Thumbnail")) {
        SetCustomThumbnailForAsset(m_selectedAssetPath);
    }
    
    // Check if asset has custom thumbnail using the same logic as thumbnail display
    bool hasCustomThumbnail = false;
    if (projectSettings) {
        AssetHandle assetHandle;
        auto assetManager = ServiceLocator::Instance().GetService<AssetManager>();
        if (assetManager) {
            assetHandle = assetManager->GetRegistry().GetAssetHandle(m_selectedAssetPath);
        }
        
        if (assetHandle.IsValid()) {
            hasCustomThumbnail = (projectSettings->GetIndividualAssetThumbnail(assetHandle) != 0);
        }
        
        if (!hasCustomThumbnail) {
            hasCustomThumbnail = (projectSettings->GetIndividualAssetThumbnailByPath(m_selectedAssetPath) != 0);
        }
    }
    
    if (hasCustomThumbnail) {
        if (ImGui::Button("Remove Custom")) {
            RemoveCustomThumbnailForAsset(m_selectedAssetPath);
        }
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Type-specific properties
    switch (m_selectedAssetType) {
        case AssetType::Texture:
            ImGui::Text("🖼️ Texture Properties");
            ImGui::Text("Format: %s", extension.c_str());
            if (m_currentAssetTextureId != 0) {
                // TODO: Get actual texture dimensions
                ImGui::Text("Preview shown above");
            }
            break;
            
        case AssetType::Material:
            ImGui::Text("🎨 Material Properties");
            // TODO: Load and display material JSON properties
            break;
            
        case AssetType::Scene:
            ImGui::Text("🌍 Scene Properties");
            // TODO: Load and display scene info
            break;
            
        case AssetType::Audio:
            ImGui::Text("🔊 Audio Properties");
            // TODO: Add audio duration, format info
            break;
            
        default:
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No additional properties available");
            break;
    }
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
    if (m_primarySelection != INVALID_ENTITY) {
        RenderComponentList(m_primarySelection);
        
        ImGui::Spacing();
        RenderAddComponentButton(m_primarySelection);
    }
}

void InspectorPanel::RenderMultiSelectionHeader() {
    ImGui::Text("🔗 Multi-Selection (%zu entities)", m_selectedEntities.size());
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "⚠ Editing affects all selected entities");
    
    if (m_primarySelection != INVALID_ENTITY) {
        auto& entityManager = EntityManager::Instance();
        auto* nameComponent = entityManager.GetComponent<NameComponent>(m_primarySelection);
        std::string displayName = nameComponent ? nameComponent->name : ("Entity " + std::to_string(m_primarySelection));
        ImGui::Text("Primary: %s", displayName.c_str());
    }
}

void InspectorPanel::RenderSingleEntityHeader(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "⚠ Invalid entity");
        return;
    }
    
    // Entity name and ID
    auto* nameComponent = entityManager.GetComponent<NameComponent>(entityId);
    std::string displayName = nameComponent ? nameComponent->name : ("Entity " + std::to_string(entityId.id));
    
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
    
    if (!entityManager.IsEntityValid(entityId)) {
        return;
    }
    
    
    // Render each component type
    auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
    if (transform) {
        RenderTransformComponent(entityId, transform);
    }
    
    auto* nameComp = entityManager.GetComponent<NameComponent>(entityId);
    if (nameComp) {
        RenderNameComponent(entityId, nameComp);
    }
    
    auto* sprite = entityManager.GetComponent<SpriteComponent>(entityId);
    if (sprite) {
        RenderSpriteComponent(entityId, sprite);
    }
    
    auto* velocity = entityManager.GetComponent<VelocityComponent>(entityId);
    if (velocity) {
        RenderVelocityComponent(entityId, velocity);
    }
    
    auto* health = entityManager.GetComponent<HealthComponent>(entityId);
    if (health) {
        RenderHealthComponent(entityId, health);
    }
    
    auto* material = entityManager.GetComponent<MaterialComponent>(entityId);
    if (material) {
        RenderMaterialComponent(entityId, material);
    }
    
    auto* light = entityManager.GetComponent<LightComponent>(entityId);
    if (light) {
        RenderLightComponent(entityId, light);
    }
    
    auto* rigidbody = entityManager.GetComponent<RigidbodyComponent>(entityId);
    if (rigidbody) {
        RenderRigidbodyComponent(entityId, rigidbody);
    }
}

void InspectorPanel::RenderTransformComponent(EntityID entityId, TransformComponent* component) {
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
    if (component->parent != INVALID_ENTITY) {
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
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedTransform = entityManager.GetComponent<TransformComponent>(selectedId);
            if (selectedTransform) {
                selectedTransform->position = component->position;
                selectedTransform->rotation = component->rotation;
                selectedTransform->scale = component->scale;
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

void InspectorPanel::RenderNameComponent(EntityID entityId, NameComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Name", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "NameComponent");
        return;
    }
    
    bool changed = InputText("Name", &component->name);
    
    // Apply changes to all selected entities if this is multi-selection
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedName = entityManager.GetComponent<NameComponent>(selectedId);
            if (selectedName) {
                selectedName->name = component->name;
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderSpriteComponent(EntityID entityId, SpriteComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Sprite", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "SpriteComponent");
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
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedSprite = entityManager.GetComponent<SpriteComponent>(selectedId);
            if (selectedSprite) {
                selectedSprite->texturePath = component->texturePath;
                selectedSprite->size = component->size;
                selectedSprite->uvOffset = component->uvOffset;
                selectedSprite->uvScale = component->uvScale;
                selectedSprite->visible = component->visible;
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderVelocityComponent(EntityID entityId, VelocityComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Velocity", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "VelocityComponent");
        return;
    }
    
    bool changed = false;
    
    changed |= InputVector3("Velocity", &component->velocity);
    changed |= InputVector3("Acceleration", &component->acceleration);
    changed |= InputFloat("Damping", &component->damping, 0.01f);
    
    // Apply changes to all selected entities
    if (changed && m_selectedEntities.size() > 1) {
        for (EntityID selectedId : m_selectedEntities) {
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedVelocity = entityManager.GetComponent<VelocityComponent>(selectedId);
            if (selectedVelocity) {
                selectedVelocity->velocity = component->velocity;
                selectedVelocity->acceleration = component->acceleration;
                selectedVelocity->damping = component->damping;
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderHealthComponent(EntityID entityId, HealthComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Health", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "HealthComponent");
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
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedHealth = entityManager.GetComponent<HealthComponent>(selectedId);
            if (selectedHealth) {
                selectedHealth->maxHealth = component->maxHealth;
                selectedHealth->currentHealth = component->currentHealth;
                selectedHealth->invulnerable = component->invulnerable;
            }
        }
    }
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderMaterialComponent(EntityID entityId, MaterialComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Material", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "MaterialComponent");
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
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedMaterial = entityManager.GetComponent<MaterialComponent>(selectedId);
            if (selectedMaterial) {
                selectedMaterial->materialID = component->materialID;
                selectedMaterial->temperature = component->temperature;
                selectedMaterial->density = component->density;
                selectedMaterial->hardness = component->hardness;
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
            CopyComponent(m_primarySelection, m_contextMenuComponent);
        }
        
        if (ImGui::MenuItem("Paste Component Values", nullptr, false, CanPasteComponent(m_contextMenuComponent))) {
            PasteComponent(m_primarySelection, m_contextMenuComponent);
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
            if (m_primarySelection != INVALID_ENTITY) {
                auto& entityManager = EntityManager::Instance();
                if (componentType == "TransformComponent") hasComponent = entityManager.GetComponent<TransformComponent>(m_primarySelection) != nullptr;
                else if (componentType == "NameComponent") hasComponent = entityManager.GetComponent<NameComponent>(m_primarySelection) != nullptr;
                else if (componentType == "SpriteComponent") hasComponent = entityManager.GetComponent<SpriteComponent>(m_primarySelection) != nullptr;
                else if (componentType == "VelocityComponent") hasComponent = entityManager.GetComponent<VelocityComponent>(m_primarySelection) != nullptr;
                else if (componentType == "HealthComponent") hasComponent = entityManager.GetComponent<HealthComponent>(m_primarySelection) != nullptr;
                else if (componentType == "MaterialComponent") hasComponent = entityManager.GetComponent<MaterialComponent>(m_primarySelection) != nullptr;
                else if (componentType == "LightComponent") hasComponent = entityManager.GetComponent<LightComponent>(m_primarySelection) != nullptr;
                else if (componentType == "RigidbodyComponent") hasComponent = entityManager.GetComponent<RigidbodyComponent>(m_primarySelection) != nullptr;
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
    
    if (!entityManager.IsEntityValid(entityId)) return;
    
    if (componentType == "TransformComponent") {
        if (!entityManager.HasComponent<TransformComponent>(entityId)) {
            TransformComponent transformComp;
            entityManager.AddComponent<TransformComponent>(entityId, std::move(transformComp));
        }
    } else if (componentType == "NameComponent") {
        if (!entityManager.HasComponent<NameComponent>(entityId)) {
            NameComponent nameComp;
            nameComp.name = "New Entity";
            entityManager.AddComponent<NameComponent>(entityId, std::move(nameComp));
        }
    } else if (componentType == "SpriteComponent") {
        if (!entityManager.HasComponent<SpriteComponent>(entityId)) {
            SpriteComponent spriteComp;
            entityManager.AddComponent<SpriteComponent>(entityId, std::move(spriteComp));
        }
    } else if (componentType == "VelocityComponent") {
        if (!entityManager.HasComponent<VelocityComponent>(entityId)) {
            VelocityComponent velocityComp;
            entityManager.AddComponent<VelocityComponent>(entityId, std::move(velocityComp));
        }
    } else if (componentType == "HealthComponent") {
        if (!entityManager.HasComponent<HealthComponent>(entityId)) {
            HealthComponent healthComp;
            entityManager.AddComponent<HealthComponent>(entityId, std::move(healthComp));
        }
    } else if (componentType == "MaterialComponent") {
        if (!entityManager.HasComponent<MaterialComponent>(entityId)) {
            MaterialComponent materialComp;
            entityManager.AddComponent<MaterialComponent>(entityId, std::move(materialComp));
        }
    } else if (componentType == "LightComponent") {
        if (!entityManager.HasComponent<LightComponent>(entityId)) {
            LightComponent lightComp;
            entityManager.AddComponent<LightComponent>(entityId, std::move(lightComp));
        }
    } else if (componentType == "RigidbodyComponent") {
        if (!entityManager.HasComponent<RigidbodyComponent>(entityId)) {
            RigidbodyComponent rigidbodyComp;
            entityManager.AddComponent<RigidbodyComponent>(entityId, std::move(rigidbodyComp));
        }
    }
    
    std::cout << "Added " << componentType << " to entity " << entityId << std::endl;
}

void InspectorPanel::RemoveComponentFromEntity(EntityID entityId, const std::string& componentType) {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) return;
    
    if (componentType == "NameComponent") {
        entityManager.RemoveComponent<NameComponent>(entityId);
    } else if (componentType == "SpriteComponent") {
        entityManager.RemoveComponent<SpriteComponent>(entityId);
    } else if (componentType == "VelocityComponent") {
        entityManager.RemoveComponent<VelocityComponent>(entityId);
    } else if (componentType == "HealthComponent") {
        entityManager.RemoveComponent<HealthComponent>(entityId);
    } else if (componentType == "MaterialComponent") {
        entityManager.RemoveComponent<MaterialComponent>(entityId);
    } else if (componentType == "LightComponent") {
        entityManager.RemoveComponent<LightComponent>(entityId);
    } else if (componentType == "RigidbodyComponent") {
        entityManager.RemoveComponent<RigidbodyComponent>(entityId);
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
        *value = static_cast<uint32_t>((std::max)(0, intValue));
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

void InspectorPanel::RenderLightComponent(EntityID entityId, LightComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Light", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "LightComponent");
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
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedLight = entityManager.GetComponent<LightComponent>(selectedId);
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
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::RenderRigidbodyComponent(EntityID entityId, RigidbodyComponent* component) {
    bool removeRequested = false;
    if (!RenderComponentHeader("Rigidbody", &removeRequested)) {
        return;
    }
    
    if (removeRequested) {
        RemoveComponentFromEntity(entityId, "RigidbodyComponent");
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
            if (selectedId == entityId) continue;
            
            auto& entityManager = EntityManager::Instance();
            auto* selectedRigidbody = entityManager.GetComponent<RigidbodyComponent>(selectedId);
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
    
    ImGui::TreePop(); // Match the TreeNodeEx in RenderComponentHeader
    ImGui::Spacing();
}

void InspectorPanel::LoadTexturePreview(const std::string& path) {
    // Clean up previous texture
    if (m_currentAssetTextureId != 0) {
        auto renderer = Services::GetRenderer();
        if (renderer) {
            renderer->DeleteTexture(m_currentAssetTextureId);
        }
        m_currentAssetTextureId = 0;
    }
    
    // Load new texture
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cerr << "Failed to load texture preview: " << path << std::endl;
        return;
    }
    
    auto renderer = Services::GetRenderer();
    if (renderer) {
        m_currentAssetTextureId = renderer->CreateTexture(width, height, 4, data);
    }
    
    stbi_image_free(data);
}

void InspectorPanel::SetCustomThumbnailForAsset(const std::string& assetPath) {
    std::string thumbnailPath = OpenNativeFileDialog();
    if (thumbnailPath.empty()) {
        return;
    }
    
    // Get asset handle from asset manager
    auto assetManager = ServiceLocator::Instance().GetService<AssetManager>();
    if (!assetManager) {
        std::cerr << "Asset manager not available" << std::endl;
        return;
    }
    
    AssetHandle assetHandle = assetManager->GetRegistry().GetAssetHandle(assetPath);
    if (!assetHandle.IsValid()) {
        std::cerr << "No valid asset handle for path: " << assetPath << std::endl;
        return;
    }
    
    // Load the thumbnail
    int width, height, channels;
    unsigned char* data = stbi_load(thumbnailPath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cerr << "Failed to load thumbnail: " << thumbnailPath << std::endl;
        return;
    }
    
    auto renderer = Services::GetRenderer();
    if (renderer) {
        // Clean up old thumbnail if exists
        auto it = m_assetThumbnails.find(assetPath);
        if (it != m_assetThumbnails.end() && it->second != 0) {
            renderer->DeleteTexture(it->second);
        }
        
        // Create new thumbnail texture
        uint32_t textureId = renderer->CreateTexture(width, height, 4, data);
        m_assetThumbnails[assetPath] = textureId;
        
        // Register with ProjectSettings using AssetHandle
        auto projectSettings = Services::GetProjectSettings();
        if (projectSettings) {
            projectSettings->SetIndividualAssetThumbnail(assetHandle, assetPath, textureId, thumbnailPath);
            // Save immediately - no need to go to Project Settings
            projectSettings->SaveProjectSettings();
        }
    }
    
    stbi_image_free(data);
}

void InspectorPanel::RemoveCustomThumbnailForAsset(const std::string& assetPath) {
    auto it = m_assetThumbnails.find(assetPath);
    if (it != m_assetThumbnails.end()) {
        if (it->second != 0) {
            auto renderer = Services::GetRenderer();
            if (renderer) {
                renderer->DeleteTexture(it->second);
            }
        }
        m_assetThumbnails.erase(it);
        
        // Remove from ProjectSettings using AssetHandle
        auto assetManager = ServiceLocator::Instance().GetService<AssetManager>();
        auto projectSettings = Services::GetProjectSettings();
        if (assetManager && projectSettings) {
            AssetHandle assetHandle = assetManager->GetRegistry().GetAssetHandle(assetPath);
            if (assetHandle.IsValid()) {
                projectSettings->RemoveIndividualAssetThumbnail(assetHandle);
                // Save immediately
                projectSettings->SaveProjectSettings();
            }
        }
    }
}

std::string InspectorPanel::OpenNativeFileDialog() {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0BMP Files\0*.bmp\0TGA Files\0*.tga\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    
    return "";
#else
    // For non-Windows platforms
    std::cerr << "Native file dialog not implemented for this platform" << std::endl;
    return "";
#endif
}

void InspectorPanel::CopyComponent(EntityID entityId, const std::string& componentType) {
    auto& entityManager = EntityManager::Instance();
    
    // Clear previous clipboard data
    s_componentClipboard.componentData.clear();
    s_componentClipboard.hasData = false;
    
    // Copy component data based on type
    if (componentType == "Transform") {
        auto* comp = entityManager.GetComponent<TransformComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(TransformComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(TransformComponent));
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Velocity") {
        auto* comp = entityManager.GetComponent<VelocityComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(VelocityComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(VelocityComponent));
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Name") {
        auto* comp = entityManager.GetComponent<NameComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            // For components with dynamic data, we need a different approach
            // For now, just store the name string
            std::string nameData = comp->name;
            s_componentClipboard.componentData.resize(nameData.size() + 1);
            std::memcpy(s_componentClipboard.componentData.data(), nameData.c_str(), nameData.size() + 1);
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Sprite") {
        auto* comp = entityManager.GetComponent<SpriteComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(SpriteComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(SpriteComponent));
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Material") {
        auto* comp = entityManager.GetComponent<MaterialComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(MaterialComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(MaterialComponent));
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Health") {
        auto* comp = entityManager.GetComponent<HealthComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(HealthComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(HealthComponent));
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Light") {
        auto* comp = entityManager.GetComponent<LightComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(LightComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(LightComponent));
            s_componentClipboard.hasData = true;
        }
    } else if (componentType == "Rigidbody") {
        auto* comp = entityManager.GetComponent<RigidbodyComponent>(entityId);
        if (comp) {
            s_componentClipboard.componentType = componentType;
            s_componentClipboard.componentData.resize(sizeof(RigidbodyComponent));
            std::memcpy(s_componentClipboard.componentData.data(), comp, sizeof(RigidbodyComponent));
            s_componentClipboard.hasData = true;
        }
    }
    
    if (s_componentClipboard.hasData) {
        std::cout << "Copied " << componentType << " component to clipboard" << std::endl;
    }
}

void InspectorPanel::PasteComponent(EntityID entityId, const std::string& componentType) {
    if (!s_componentClipboard.hasData || s_componentClipboard.componentType != componentType) {
        return;
    }
    
    auto& entityManager = EntityManager::Instance();
    
    // Paste component data based on type
    if (componentType == "Transform") {
        auto* comp = entityManager.GetComponent<TransformComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(TransformComponent)) {
            TransformComponent* clipboardData = reinterpret_cast<TransformComponent*>(s_componentClipboard.componentData.data());
            // Copy position, rotation, scale but not parent/children relationships
            comp->position = clipboardData->position;
            comp->rotation = clipboardData->rotation;
            comp->scale = clipboardData->scale;
        }
    } else if (componentType == "Velocity") {
        auto* comp = entityManager.GetComponent<VelocityComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(VelocityComponent)) {
            std::memcpy(comp, s_componentClipboard.componentData.data(), sizeof(VelocityComponent));
        }
    } else if (componentType == "Name") {
        auto* comp = entityManager.GetComponent<NameComponent>(entityId);
        if (comp && !s_componentClipboard.componentData.empty()) {
            comp->name = std::string(reinterpret_cast<const char*>(s_componentClipboard.componentData.data()));
        }
    } else if (componentType == "Sprite") {
        auto* comp = entityManager.GetComponent<SpriteComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(SpriteComponent)) {
            std::memcpy(comp, s_componentClipboard.componentData.data(), sizeof(SpriteComponent));
        }
    } else if (componentType == "Material") {
        auto* comp = entityManager.GetComponent<MaterialComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(MaterialComponent)) {
            std::memcpy(comp, s_componentClipboard.componentData.data(), sizeof(MaterialComponent));
        }
    } else if (componentType == "Health") {
        auto* comp = entityManager.GetComponent<HealthComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(HealthComponent)) {
            std::memcpy(comp, s_componentClipboard.componentData.data(), sizeof(HealthComponent));
        }
    } else if (componentType == "Light") {
        auto* comp = entityManager.GetComponent<LightComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(LightComponent)) {
            std::memcpy(comp, s_componentClipboard.componentData.data(), sizeof(LightComponent));
        }
    } else if (componentType == "Rigidbody") {
        auto* comp = entityManager.GetComponent<RigidbodyComponent>(entityId);
        if (comp && s_componentClipboard.componentData.size() == sizeof(RigidbodyComponent)) {
            std::memcpy(comp, s_componentClipboard.componentData.data(), sizeof(RigidbodyComponent));
        }
    }
    
    // Apply to all selected entities if multi-selection
    if (m_selectedEntities.size() > 1) {
        for (EntityID entity : m_selectedEntities) {
            if (entity != entityId) {
                PasteComponent(entity, componentType);
            }
        }
    }
    
    std::cout << "Pasted " << componentType << " component from clipboard" << std::endl;
}

bool InspectorPanel::CanPasteComponent(const std::string& componentType) const {
    return s_componentClipboard.hasData && s_componentClipboard.componentType == componentType;
}

} // namespace BGE