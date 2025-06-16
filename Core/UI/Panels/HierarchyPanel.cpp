#include "HierarchyPanel.h"
#include "../../Services.h"
#include "../../Components.h"
#include "../../Entity.h"
#include "../../ServiceLocator.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace BGE {

HierarchyPanel::HierarchyPanel(const std::string& name, SimulationWorld* world)
    : Panel(name, PanelDockPosition::Left)
    , m_world(world) {
}

HierarchyPanel::~HierarchyPanel() {
    UnregisterEventListeners();
}

void HierarchyPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
    RegisterEventListeners();
}

void HierarchyPanel::RegisterEventListeners() {
    // Get event bus from service locator
    auto eventBusPtr = ServiceLocator::Instance().GetService<EventBus>();
    m_eventBus = eventBusPtr.get();
    
    if (m_eventBus) {
        // Listen for external selection changes
        m_eventBus->Subscribe<EntitySelectionChangedEvent>([this](const EntitySelectionChangedEvent& event) {
            OnEntitySelectionChanged(event);
        });
    }
}

void HierarchyPanel::UnregisterEventListeners() {
    // EventBus will handle cleanup when it's destroyed
}

void HierarchyPanel::OnRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.0f);
    
    // Handle keyboard input for the panel
    HandleKeyboardInput();
    
    // Render the main hierarchy
    RenderEntityHierarchy();
    
    // Handle right-click in empty space for context menu
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
        m_contextMenuEntity = INVALID_ENTITY_ID;
        ImGui::OpenPopup("HierarchyContextMenu");
    }
    
    // Show context menu
    ShowContextMenu();
    
    // Clear selection on empty area click
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        ClearSelection();
    }
    
    ImGui::PopStyleVar(2);
}

void HierarchyPanel::RenderEntityHierarchy() {
    auto& entityManager = EntityManager::Instance();
    
    const auto& allEntities = entityManager.GetAllEntities();
    
    // Early return if no entities
    if (allEntities.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No entities in scene");
        return;
    }
    
    // Render all root entities (entities without parents)
    RenderRootEntities();
}

void HierarchyPanel::RenderRootEntities() {
    std::vector<EntityID> rootEntities = GetRootEntities();
    
    for (EntityID entityId : rootEntities) {
        bool nodeOpen = false;
        RenderEntityNode(entityId, nodeOpen);
    }
}

void HierarchyPanel::RenderEntityNode(EntityID entityId, bool& nodeOpen) {
    auto& entityManager = EntityManager::Instance();
    auto entity = entityManager.GetEntity(entityId);
    
    if (!entity) {
        return; // Entity was destroyed
    }
    
    std::string displayName = GetEntityDisplayName(entityId);
    const char* icon = GetEntityIcon(entityId);
    bool hasChildren = HasChildren(entityId);
    bool isSelected = IsEntitySelected(entityId);
    bool isExpanded = m_expandedNodes.count(entityId) > 0;
    
    // Create the tree node
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    
    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    if (isExpanded) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    
    // Always push ID first for consistency
    ImGui::PushID(static_cast<int>(entityId));
    
    // Handle renaming mode
    if (m_renamingEntity == entityId) {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                           ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            RenameEntity(entityId, m_renameBuffer);
            m_renamingEntity = INVALID_ENTITY_ID;
        }
        
        // Cancel renaming on escape or focus loss
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(256)) || !ImGui::IsItemActive()) { // 256 = Escape
            m_renamingEntity = INVALID_ENTITY_ID;
        }
        
        // Pop ID and return
        ImGui::PopID();
        return;
    }
    
    // Render the tree node
    std::string nodeLabel = std::string(icon) + " " + displayName;
    nodeOpen = ImGui::TreeNodeEx("node", flags, "%s", nodeLabel.c_str());
    
    // Handle selection
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        bool ctrlHeld = ImGui::GetIO().KeyCtrl;
        bool shiftHeld = ImGui::GetIO().KeyShift;
        SelectEntity(entityId, ctrlHeld, shiftHeld);
    }
    
    // Handle double-click for renaming
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        m_renamingEntity = entityId;
        strncpy(m_renameBuffer, displayName.c_str(), sizeof(m_renameBuffer) - 1);
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }
    
    // Handle right-click context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        m_contextMenuEntity = entityId;
        ImGui::OpenPopup("HierarchyContextMenu");
        if (!IsEntitySelected(entityId)) {
            SelectEntity(entityId, false, false); // Select the right-clicked entity
        }
    }
    
    // Handle drag and drop
    HandleDragAndDrop(entityId);
    
    // Handle material drag and drop
    HandleMaterialDragAndDrop(entityId);
    
    // Update expansion state
    if (hasChildren) {
        if (nodeOpen && m_expandedNodes.count(entityId) == 0) {
            m_expandedNodes.insert(entityId);
        } else if (!nodeOpen && m_expandedNodes.count(entityId) > 0) {
            m_expandedNodes.erase(entityId);
        }
    }
    
    // Render children if expanded
    if (nodeOpen && hasChildren) {
        std::vector<EntityID> children = GetChildEntities(entityId);
        for (EntityID childId : children) {
            bool childNodeOpen = false;
            RenderEntityNode(childId, childNodeOpen);
        }
    }
    
    // Only pop if node was opened AND we actually pushed (i.e., has children)
    if (nodeOpen && hasChildren) {
        ImGui::TreePop();
    }
    
    // Pop the ID we pushed at the beginning
    ImGui::PopID();
}

std::string HierarchyPanel::GetEntityDisplayName(EntityID entityId) const {
    auto& entityManager = EntityManager::Instance();
    auto entity = entityManager.GetEntity(entityId);
    
    if (!entity) {
        return "Invalid Entity";
    }
    
    auto* nameComponent = entity->GetComponent<NameComponent>();
    if (nameComponent && !nameComponent->name.empty()) {
        return nameComponent->name;
    }
    
    return "Entity " + std::to_string(entityId);
}

const char* HierarchyPanel::GetEntityIcon(EntityID entityId) const {
    auto& entityManager = EntityManager::Instance();
    auto entity = entityManager.GetEntity(entityId);
    
    if (!entity) {
        return "?";
    }
    
    // Check for specific component types and return appropriate icons
    if (entity->GetComponent<VelocityComponent>()) {
        return ">"; // Moving object
    }
    
    if (entity->GetComponent<MaterialComponent>()) {
        return "M"; // Material object
    }
    
    if (entity->GetComponent<SpriteComponent>()) {
        return "S"; // Sprite
    }
    
    auto* name = entity->GetComponent<NameComponent>();
    if (name) {
        std::string nameStr = name->name;
        std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
        
        if (nameStr.find("camera") != std::string::npos) {
            return "C";
        }
        if (nameStr.find("light") != std::string::npos) {
            return "L";
        }
        if (nameStr.find("player") != std::string::npos) {
            return "P";
        }
    }
    
    return "E"; // Default entity icon
}

void HierarchyPanel::SelectEntity(EntityID entityId, bool ctrlHeld, bool shiftHeld) {
    
    if (shiftHeld && m_lastClickedEntity != INVALID_ENTITY_ID) {
        // Shift-click: range selection
        SelectRange(m_lastClickedEntity, entityId);
    } else if (ctrlHeld) {
        // Ctrl-click: toggle selection
        if (IsEntitySelected(entityId)) {
            m_selectedEntities.erase(entityId);
            if (m_primarySelection == entityId) {
                m_primarySelection = m_selectedEntities.empty() ? INVALID_ENTITY_ID : *m_selectedEntities.begin();
            }
        } else {
            m_selectedEntities.insert(entityId);
            m_primarySelection = entityId;
        }
    } else {
        // Normal click: single selection
        m_selectedEntities.clear();
        m_selectedEntities.insert(entityId);
        m_primarySelection = entityId;
    }
    
    m_lastClickedEntity = entityId;
    BroadcastSelectionChanged();
}

void HierarchyPanel::ClearSelection() {
    m_selectedEntities.clear();
    m_primarySelection = INVALID_ENTITY_ID;
    m_lastClickedEntity = INVALID_ENTITY_ID;
    BroadcastSelectionChanged();
}

bool HierarchyPanel::IsEntitySelected(EntityID entityId) const {
    return m_selectedEntities.count(entityId) > 0;
}

void HierarchyPanel::BroadcastSelectionChanged() {
    if (!m_eventBus) {
        return;
    }
    
    std::vector<EntityID> selectedVector(m_selectedEntities.begin(), m_selectedEntities.end());
    EntitySelectionChangedEvent event(selectedVector);
    event.primarySelection = m_primarySelection;
    
    m_eventBus->Publish(event);
}

void HierarchyPanel::OnEntitySelectionChanged(const EntitySelectionChangedEvent& event) {
    // Update our selection to match external changes (e.g., from Scene View)
    m_selectedEntities.clear();
    for (EntityID id : event.selectedEntities) {
        m_selectedEntities.insert(id);
    }
    m_primarySelection = event.primarySelection;
    
    if (!event.selectedEntities.empty()) {
        m_lastClickedEntity = event.selectedEntities.back();
    }
}

std::vector<EntityID> HierarchyPanel::GetRootEntities() const {
    std::vector<EntityID> rootEntities;
    auto& entityManager = EntityManager::Instance();
    
    for (const auto& [id, entity] : entityManager.GetAllEntities()) {
        auto* transform = entity->GetComponent<TransformComponent>();
        if (!transform || transform->parent == INVALID_ENTITY_ID) {
            rootEntities.push_back(id);
        }
    }
    
    return rootEntities;
}

std::vector<EntityID> HierarchyPanel::GetChildEntities(EntityID parentId) const {
    auto& entityManager = EntityManager::Instance();
    auto parentEntity = entityManager.GetEntity(parentId);
    
    if (!parentEntity) {
        return {};
    }
    
    auto* transform = parentEntity->GetComponent<TransformComponent>();
    if (!transform) {
        return {};
    }
    
    return transform->children;
}

bool HierarchyPanel::HasChildren(EntityID entityId) const {
    return !GetChildEntities(entityId).empty();
}

void HierarchyPanel::HandleKeyboardInput() {
    if (!ImGui::IsWindowFocused()) return;
    
    // F2 or Enter: rename selected entity
    if ((ImGui::IsKeyPressed(static_cast<ImGuiKey>(291)) || ImGui::IsKeyPressed(static_cast<ImGuiKey>(257))) && // 291 = F2, 257 = Enter
        m_primarySelection != INVALID_ENTITY_ID && m_renamingEntity == INVALID_ENTITY_ID) {
        m_renamingEntity = m_primarySelection;
        std::string currentName = GetEntityDisplayName(m_primarySelection);
        strncpy(m_renameBuffer, currentName.c_str(), sizeof(m_renameBuffer) - 1);
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }
    
    // Delete: delete selected entities
    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(261)) && !m_selectedEntities.empty()) { // 261 = Delete
        DeleteSelectedEntities();
    }
    
    // Ctrl+D: duplicate selected entities
    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>('D')) && ImGui::GetIO().KeyCtrl && !m_selectedEntities.empty()) {
        DuplicateSelectedEntities();
    }
}

void HierarchyPanel::RenameEntity(EntityID entityId, const std::string& newName) {
    auto& entityManager = EntityManager::Instance();
    auto entity = entityManager.GetEntity(entityId);
    
    if (!entity) return;
    
    auto* nameComponent = entity->GetComponent<NameComponent>();
    if (!nameComponent) {
        // Add name component if it doesn't exist
        nameComponent = entity->AddComponent<NameComponent>();
    }
    
    nameComponent->name = newName;
    std::cout << "Renamed entity " << entityId << " to '" << newName << "'" << std::endl;
}

void HierarchyPanel::DeleteSelectedEntities() {
    auto& entityManager = EntityManager::Instance();
    
    for (EntityID entityId : m_selectedEntities) {
        std::cout << "Deleting entity " << entityId << std::endl;
        entityManager.DestroyEntity(entityId);
    }
    
    ClearSelection();
}

void HierarchyPanel::DuplicateSelectedEntities() {
    // TODO: Implement entity duplication
    // This would require a proper entity cloning system
    std::cout << "Entity duplication not yet implemented" << std::endl;
}

void HierarchyPanel::CreateChildEntity(EntityID parentId) {
    auto& entityManager = EntityManager::Instance();
    
    // Create new entity
    auto newEntity = entityManager.CreateEntity();
    EntityID newEntityId = newEntity->GetID();
    
    if (!newEntity) return;
    
    // Add basic components
    auto* nameComponent = newEntity->AddComponent<NameComponent>();
    nameComponent->name = "New Entity";
    
    auto* transform = newEntity->AddComponent<TransformComponent>();
    
    // Set parent relationship if specified
    if (parentId != INVALID_ENTITY_ID) {
        auto parentEntity = entityManager.GetEntity(parentId);
        if (parentEntity) {
            auto* parentTransform = parentEntity->GetComponent<TransformComponent>();
            if (parentTransform) {
                transform->parent = parentId;
                parentTransform->children.push_back(newEntityId);
                
                // Expand the parent node
                m_expandedNodes.insert(parentId);
            }
        }
    }
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
    
    std::cout << "Created new entity " << newEntityId << " with parent " << parentId << std::endl;
}

void HierarchyPanel::CreateEmpty() {
    auto& entityManager = EntityManager::Instance();
    
    // Create new empty entity
    auto newEntity = entityManager.CreateEntity("Empty");
    if (!newEntity) return;
    
    EntityID newEntityId = newEntity->GetID();
    
    // Add basic components
    newEntity->AddComponent<NameComponent>("Empty");
    newEntity->AddComponent<TransformComponent>();
    
    // Add visual components so it appears in the scene
    newEntity->AddComponent<SpriteComponent>();
    auto* material = newEntity->AddComponent<MaterialComponent>();
    material->materialID = 1; // Default material
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreateRigidbodyEntity() {
    auto& entityManager = EntityManager::Instance();
    
    // Create new entity with rigidbody
    auto newEntity = entityManager.CreateEntity("Rigidbody");
    EntityID newEntityId = newEntity->GetID();
    
    if (!newEntity) return;
    
    // Add required components
    newEntity->AddComponent<NameComponent>("Rigidbody");
    newEntity->AddComponent<TransformComponent>();
    
    // Add visual components so it appears in the scene
    newEntity->AddComponent<SpriteComponent>();
    auto* material = newEntity->AddComponent<MaterialComponent>();
    material->materialID = 2; // Different material for rigidbody objects
    
    // Add rigidbody component with sensible defaults
    auto* rigidbody = newEntity->AddComponent<RigidbodyComponent>();
    rigidbody->mass = 1.0f;
    rigidbody->useGravity = true;
    rigidbody->drag = 0.0f;
    rigidbody->angularDrag = 0.05f;
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
    
    std::cout << "Created rigidbody entity " << newEntityId << " with physics and visual components" << std::endl;
}

void HierarchyPanel::CreatePointLightEntity() {
    auto& entityManager = EntityManager::Instance();
    
    // Create new entity with point light
    auto newEntity = entityManager.CreateEntity("Point Light");
    EntityID newEntityId = newEntity->GetID();
    
    if (!newEntity) return;
    
    // Add required components
    newEntity->AddComponent<NameComponent>("Point Light");
    newEntity->AddComponent<TransformComponent>();
    
    // Add visual components (lights should also be visible)
    newEntity->AddComponent<SpriteComponent>();
    auto* material = newEntity->AddComponent<MaterialComponent>();
    material->materialID = 3; // Light material (could be glowing/bright)
    
    // Add light component with sensible defaults for point light
    auto* light = newEntity->AddComponent<LightComponent>(LightComponent::Point);
    light->color = Vector3{1.0f, 1.0f, 1.0f};  // White light
    light->intensity = 1.0f;
    light->range = 10.0f;
    light->enabled = true;
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
    
    std::cout << "Created point light entity " << newEntityId << " with light and visual components" << std::endl;
}

void HierarchyPanel::ShowContextMenu() {
    if (ImGui::BeginPopup("HierarchyContextMenu")) {
        if (m_contextMenuEntity != INVALID_ENTITY_ID) {
            std::string entityName = GetEntityDisplayName(m_contextMenuEntity);
            ImGui::Text("Entity: %s", entityName.c_str());
            ImGui::Separator();
            
            if (ImGui::MenuItem("Create Empty Child")) {
                CreateChildEntity(m_contextMenuEntity);
            }
            
            if (ImGui::MenuItem("Duplicate")) {
                DuplicateSelectedEntities();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Rename", "F2")) {
                m_renamingEntity = m_contextMenuEntity;
                std::string currentName = GetEntityDisplayName(m_contextMenuEntity);
                strncpy(m_renameBuffer, currentName.c_str(), sizeof(m_renameBuffer) - 1);
                m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Delete", "Del")) {
                DeleteSelectedEntities();
            }
        } else {
            // Context menu in empty space - object creation menu
            if (ImGui::MenuItem("Create Empty")) {
                CreateEmpty();
            }
            
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::BeginMenu("Physics")) {
                    if (ImGui::MenuItem("Rigidbody")) {
                        CreateRigidbodyEntity();
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Light")) {
                    if (ImGui::MenuItem("Point Light")) {
                        CreatePointLightEntity();
                    }
                    ImGui::EndMenu();
                }
                
                ImGui::EndMenu();
            }
        }
        
        ImGui::EndPopup();
    }
}

void HierarchyPanel::HandleDragAndDrop(EntityID /*entityId*/) {
    // TODO: Implement drag and drop reparenting
    // This would involve:
    // 1. Detecting drag start on entity
    // 2. Showing drag preview
    // 3. Detecting drop targets
    // 4. Updating parent-child relationships
}

void HierarchyPanel::HandleMaterialDragAndDrop(EntityID entityId) {
    // Handle material drop from AssetBrowser
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            std::string draggedAsset = static_cast<const char*>(payload->Data);
            
            // Check if the dragged asset is a material
            std::filesystem::path assetPath(draggedAsset);
            std::string extension = assetPath.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            if (extension == ".json") {
                // Check if it's a material file
                std::string filename = assetPath.stem().string();
                std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                
                if (filename.find("material") != std::string::npos || filename.find("mat") != std::string::npos) {
                    // Apply material to entity
                    ApplyMaterialToEntity(entityId, draggedAsset);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyPanel::ApplyMaterialToEntity(EntityID entityId, const std::string& materialPath) {
    auto& entityManager = EntityManager::Instance();
    auto entity = entityManager.GetEntity(entityId);
    
    if (!entity) {
        std::cout << "Failed to apply material: Entity " << entityId << " not found" << std::endl;
        return;
    }
    
    // Load material ID from file or assign a default
    uint32_t materialID = 1; // Default material ID
    
    try {
        // Try to extract material ID from the material file
        std::ifstream file(materialPath);
        if (file.is_open()) {
            // Simple material ID extraction - in a real implementation,
            // you'd parse the JSON and extract the material ID
            std::string filename = std::filesystem::path(materialPath).stem().string();
            
            // Generate a simple material ID based on filename hash
            // In a real implementation, this would be loaded from the material system
            std::hash<std::string> hasher;
            materialID = static_cast<uint32_t>(hasher(filename) % 1000) + 1;
        }
    } catch (const std::exception& e) {
        std::cout << "Warning: Could not read material file " << materialPath << ": " << e.what() << std::endl;
    }
    
    // Get or add MaterialComponent
    auto* materialComponent = entity->GetComponent<MaterialComponent>();
    if (!materialComponent) {
        materialComponent = entity->AddComponent<MaterialComponent>();
    }
    
    // Apply the new material
    materialComponent->materialID = materialID;
    
    std::cout << "Applied material " << materialPath << " (ID: " << materialID << ") to entity " << entityId << std::endl;
}

void HierarchyPanel::SelectRange(EntityID /*fromEntity*/, EntityID /*toEntity*/) {
    // TODO: Implement range selection
    // This would require determining the visual order of entities in the hierarchy
    std::cout << "Range selection not yet implemented" << std::endl;
}

std::vector<EntityID> HierarchyPanel::GetEntitiesBetween(EntityID /*start*/, EntityID /*end*/) const {
    // TODO: Implement getting entities between two entities in hierarchy order
    return {};
}

} // namespace BGE