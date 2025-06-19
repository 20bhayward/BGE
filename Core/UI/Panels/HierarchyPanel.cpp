#include "HierarchyPanel.h"
#include "../../Services.h"
#include "../../Components.h"
#include "../../Entity.h"
#include "../../ServiceLocator.h"
#include "../../ECS/EntityManager.h"
#include "../../ECS/EntityQuery.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>

namespace BGE {

// Static clipboard definition
HierarchyPanel::EntityClipboardData HierarchyPanel::s_clipboard;

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
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, m_indentSize);
    
    // Search bar
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    
    bool searchChanged = false;
    ImGui::PushID("HierarchySearch");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search...", m_searchBuffer, sizeof(m_searchBuffer))) {
        searchChanged = true;
        m_searchQuery = m_searchBuffer;
        UpdateSearchResults();
    }
    ImGui::PopID();
    
    // Clear search button
    if (!m_searchQuery.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(m_clearButtonWidth, 0))) {
            m_searchBuffer[0] = '\0';
            m_searchQuery.clear();
            m_searchResults.clear();
            m_showOnlySearchResults = false;
        }
    }
    
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
    
    ImGui::Separator();
    
    // Toolbar with create button
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
    
    if (ImGui::Button("Create")) {
        ImGui::OpenPopup("CreateMenu");
    }
    
    ImGui::PopStyleVar();
    ImGui::EndGroup();
    
    // Create menu popup
    if (ImGui::BeginPopup("CreateMenu")) {
        if (ImGui::MenuItem("Empty GameObject")) {
            CreateEmpty();
        }
        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Point Light")) { CreatePointLightEntity(); }
            if (ImGui::MenuItem("Directional Light")) { CreateDirectionalLightEntity(); }
            if (ImGui::MenuItem("Spot Light")) { CreateSpotLightEntity(); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Effects")) {
            if (ImGui::MenuItem("Particle System")) { CreateParticleSystemEntity(); }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    // Handle keyboard input for the panel
    HandleKeyboardInput();
    
    // Handle keyboard shortcuts when panel is focused
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        HandleKeyboardShortcuts();
    }
    
    // Begin scrolling region for hierarchy
    ImGui::BeginChild("HierarchyTree", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), false);
    
    // Render the main hierarchy
    RenderEntityHierarchy();
    
    // Handle right-click in empty space for context menu
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
        m_contextMenuEntity = INVALID_ENTITY;
        ImGui::OpenPopup("HierarchyContextMenu");
    }
    
    // Show context menu
    ShowContextMenu();
    
    // Clear selection on empty area click
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        ClearSelection();
    }
    
    ImGui::EndChild();
    
    // Status bar
    ImGui::Separator();
    ImGui::Text("%zu entities | %zu selected | %zu hidden | %zu locked", 
                m_stats.totalEntities, m_stats.selectedEntities, 
                m_hiddenEntities.size(), m_lockedEntities.size());
    
    ImGui::PopStyleVar(2);
}

void HierarchyPanel::RenderEntityHierarchy() {
    auto& entityManager = EntityManager::Instance();
    
    // Check if hierarchy needs update
    const auto allEntityIDs = entityManager.GetAllEntityIDs();
    if (allEntityIDs.size() != m_lastEntityCount) {
        m_hierarchyDirty = true;
        m_lastEntityCount = static_cast<uint32_t>(allEntityIDs.size());
    }
    
    // Update caches if needed
    if (m_hierarchyDirty) {
        UpdateCaches();
    }
    
    // Update stats
    m_stats.totalEntities = allEntityIDs.size();
    m_stats.selectedEntities = m_selectedEntities.size();
    m_stats.visibleEntities = 0;
    m_stats.lockedEntities = m_lockedEntities.size();
    
    // Early return if no entities
    if (allEntityIDs.empty()) {
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
    
    // Don't use GetEntity - it returns nullptr for new ECS entities
    if (!entityManager.IsEntityValid(entityId)) {
        return; // Entity was destroyed
    }
    
    // Check if entity should be visible based on search filter
    if (!m_searchQuery.empty() && !IsVisible(entityId)) {
        return;
    }
    
    // Update visible entity count
    m_stats.visibleEntities++;
    
    std::string displayName = GetEntityDisplayName(entityId);
    const char* icon = GetEntityIcon(entityId);
    bool hasChildren = HasChildren(entityId);
    bool isSelected = IsEntitySelected(entityId);
    bool isExpanded = m_expandedNodes.count(entityId) > 0;
    bool isVisible = IsEntityVisible(entityId);
    bool isLocked = IsEntityLocked(entityId);
    
    // Create the tree node
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    
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
    
    // Store item rect for drag & drop visualization
    ImVec2 nodePos = ImGui::GetCursorScreenPos();
    
    // Handle renaming mode
    if (m_renamingEntity == entityId) {
        // Add some padding to align with tree node
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());
        
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                           ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            EntityOperation op;
            op.type = EntityOperation::Rename;
            op.entityId = entityId;
            op.oldName = displayName;
            op.newName = m_renameBuffer;
            RecordOperation(op);
            
            RenameEntity(entityId, m_renameBuffer);
            m_renamingEntity = INVALID_ENTITY;
        }
        
        // Cancel renaming on escape or focus loss
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || !ImGui::IsItemActive()) {
            m_renamingEntity = INVALID_ENTITY;
        }
        
        // Pop ID and return
        ImGui::PopID();
        return;
    }
    
    // Calculate node width for controls
    float nodeWidth = ImGui::GetContentRegionAvail().x;
    float controlsWidth = 0;
    if (m_showVisibilityToggles) controlsWidth += m_visibilityButtonWidth;
    if (m_showLockToggles) controlsWidth += m_lockButtonWidth;
    
    // Save cursor position for later
    ImVec2 nodeStartPos = ImGui::GetCursorPos();
    
    // First, reserve space and draw the control buttons on the right
    if (controlsWidth > 0) {
        float buttonsStartX = nodeWidth - controlsWidth;
        
        // Visibility toggle
        if (m_showVisibilityToggles) {
            ImGui::SetCursorPosX(nodeStartPos.x + buttonsStartX);
            ImGui::PushID("vis");
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::PushStyleColor(ImGuiCol_Text, isVisible ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            if (ImGui::SmallButton(isVisible ? "O" : "X")) {
                ToggleVisibility(entityId);
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(isVisible ? "Click to hide entity" : "Click to show entity");
            }
            ImGui::PopID();
            buttonsStartX += m_visibilityButtonWidth;
        }
        
        // Lock toggle
        if (m_showLockToggles) {
            ImGui::SameLine();
            ImGui::SetCursorPosX(nodeStartPos.x + buttonsStartX);
            ImGui::PushID("lock");
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::PushStyleColor(ImGuiCol_Text, isLocked ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::SmallButton(isLocked ? "L" : "U")) {
                ToggleLock(entityId);
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(isLocked ? "Click to unlock entity" : "Click to lock entity");
            }
            ImGui::PopID();
        }
        
        // Return cursor to start position for tree node
        ImGui::SameLine();
        ImGui::SetCursorPosX(nodeStartPos.x);
    }
    
    // Render the tree node
    std::string nodeLabel = displayName;  // We'll handle icon separately
    
    // Apply visual style based on state
    if (!isVisible) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    }
    if (isLocked) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    }
    
    // Highlight search results
    if (!m_searchQuery.empty() && MatchesSearchFilter(entityId)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
    }
    
    // Override selection color to blue instead of yellow
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.31f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.67f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
    
    // Set tree node to not use full width
    ImGui::PushItemWidth(nodeWidth - controlsWidth - 10);
    
    // Draw icon with color if enabled
    if (m_showIcons) {
        ImVec4 iconColor = GetEntityIconColor(entityId);
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::Text("%s", icon);
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 4); // Small spacing between icon and text
    }
    
    nodeOpen = ImGui::TreeNodeEx("node", flags, "%s", nodeLabel.c_str());
    ImGui::PopItemWidth();
    
    // Pop selection colors
    ImGui::PopStyleColor(3);
    
    // Store if the tree node was clicked
    bool treeNodeClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool treeNodeRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    bool treeNodeHovered = ImGui::IsItemHovered();
    
    // Pop text colors
    if (!m_searchQuery.empty() && MatchesSearchFilter(entityId)) {
        ImGui::PopStyleColor();
    }
    if (isLocked) {
        ImGui::PopStyleColor();
    }
    if (!isVisible) {
        ImGui::PopStyleColor();
    }
    
    // Store rect for drag visualization
    ImRect itemRect(nodePos, ImVec2(nodePos.x + nodeWidth, nodePos.y + ImGui::GetFrameHeight()));
    
    // Handle selection and double-click
    if (treeNodeClicked && !isLocked) {
        static float lastClickTime = 0.0f;
        static EntityID lastClickedEntity = INVALID_ENTITY;
        float currentTime = static_cast<float>(ImGui::GetTime());
        
        // Check for double-click
        if (lastClickedEntity == entityId && (currentTime - lastClickTime) < m_doubleClickTime) {
            // Double-click detected - focus camera on entity
            FocusCameraOnEntity(entityId);
        } else {
            // Single click - handle selection
            bool ctrlHeld = ImGui::GetIO().KeyCtrl;
            bool shiftHeld = ImGui::GetIO().KeyShift;
            SelectEntity(entityId, ctrlHeld, shiftHeld);
        }
        
        lastClickTime = currentTime;
        lastClickedEntity = entityId;
    }
    
    // Handle right-click context menu
    if (treeNodeRightClicked) {
        m_contextMenuEntity = entityId;
        ImGui::OpenPopup("HierarchyContextMenu");
        if (!IsEntitySelected(entityId)) {
            SelectEntity(entityId, false, false); // Select the right-clicked entity
        }
    }
    
    // Handle drag and drop on the tree node itself
    if (!isLocked && treeNodeHovered) {
        // Start dragging
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            // Store the entity ID being dragged
            ImGui::SetDragDropPayload("ENTITY_ID", &entityId, sizeof(EntityID));
            
            // Show preview with icon
            std::string entityName = GetEntityDisplayName(entityId);
            const char* dragIcon = GetEntityIcon(entityId);
            ImGui::Text("%s %s", dragIcon, entityName.c_str());
            
            // If multiple entities selected, show count
            if (m_selectedEntities.size() > 1 && m_selectedEntities.count(entityId) > 0) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(+%zu more)", m_selectedEntities.size() - 1);
            }
            
            ImGui::EndDragDropSource();
        }
    }
    
    // Accept drops with custom styling
    ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Make default highlight transparent
    if (ImGui::BeginDragDropTarget()) {
        // Update drop position and target for visualization
        ImVec2 mousePos = ImGui::GetMousePos();
        m_dropTargetEntity = entityId;
        m_dropTargetRect = itemRect;
        m_currentDropPosition = GetDropPosition(mousePos, itemRect);
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID")) {
            EntityID draggedEntity = *(const EntityID*)payload->Data;
            
            // Process the drop
            HandleDragDropPayload(entityId, draggedEntity);
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopStyleColor();
    
    // Clear drop target if mouse released
    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_dropTargetEntity != INVALID_ENTITY) {
        m_dropTargetEntity = INVALID_ENTITY;
    }
    
    // Draw drop indicator if this is the current drop target
    if (m_dropTargetEntity == entityId && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        DrawDropIndicator(m_currentDropPosition, itemRect);
    }
    
    // Handle material drag and drop
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
    // Check cache first
    auto it = m_cachedDisplayNames.find(entityId);
    if (it != m_cachedDisplayNames.end()) {
        return it->second;
    }
    
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
        return "Invalid Entity";
    }
    
    std::string displayName;
    auto* nameComponent = entityManager.GetComponent<NameComponent>(entityId);
    if (nameComponent && !nameComponent->name.empty()) {
        displayName = nameComponent->name;
    } else {
        displayName = "Entity " + std::to_string(entityId.id);
    }
    
    // Cache the result
    m_cachedDisplayNames[entityId] = displayName;
    return displayName;
}

const char* HierarchyPanel::GetEntityIcon(EntityID entityId) const {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
        return "[X]";
    }
    
    // Check for specific component types and return appropriate text icons
    
    // Light components get priority
    if (entityManager.HasComponent<LightComponent>(entityId)) {
        auto* light = entityManager.GetComponent<LightComponent>(entityId);
        if (light) {
            switch (light->type) {
                case LightComponent::Point: return "[o]"; // Light bulb
                case LightComponent::Directional: return "[*]"; // Sun
                case LightComponent::Spot: return "[V]"; // Cone
                default: return "[o]";
            }
        }
    }
    
    // Camera
    auto* name = entityManager.GetComponent<NameComponent>(entityId);
    if (name) {
        std::string nameStr = name->name;
        std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
        
        if (nameStr.find("camera") != std::string::npos) {
            return "[C]"; // Camera
        }
        if (nameStr.find("player") != std::string::npos) {
            return "[P]"; // Player
        }
        if (nameStr.find("enemy") != std::string::npos) {
            return "[E]"; // Enemy
        }
        if (nameStr.find("wall") != std::string::npos || nameStr.find("ground") != std::string::npos) {
            return "[#]"; // Solid object
        }
    }
    
    // Physics objects
    if (entityManager.HasComponent<RigidbodyComponent>(entityId)) {
        return "[R]"; // Rigidbody
    }
    
    // Moving objects
    if (entityManager.HasComponent<VelocityComponent>(entityId)) {
        return "[>]"; // Moving
    }
    
    // Visual objects
    if (entityManager.HasComponent<SpriteComponent>(entityId)) {
        return "[S]"; // Sprite
    }
    
    if (entityManager.HasComponent<MaterialComponent>(entityId)) {
        return "[M]"; // Material
    }
    
    // Parent objects (containers)
    if (HasChildren(entityId)) {
        return "[+]"; // Folder/Parent
    }
    
    return "[ ]"; // Default entity icon
}

ImVec4 HierarchyPanel::GetEntityIconColor(EntityID entityId) const {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for invalid
    }
    
    // Light components - yellow/orange
    if (entityManager.HasComponent<LightComponent>(entityId)) {
        auto* light = entityManager.GetComponent<LightComponent>(entityId);
        if (light) {
            switch (light->type) {
                case LightComponent::Point: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                case LightComponent::Directional: return ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Orange
                case LightComponent::Spot: return ImVec4(1.0f, 0.9f, 0.3f, 1.0f); // Light orange
                default: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            }
        }
    }
    
    // Check name for special types
    auto* name = entityManager.GetComponent<NameComponent>(entityId);
    if (name) {
        std::string nameStr = name->name;
        std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
        
        if (nameStr.find("camera") != std::string::npos) {
            return ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Light blue
        }
        if (nameStr.find("player") != std::string::npos) {
            return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        }
        if (nameStr.find("enemy") != std::string::npos) {
            return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        }
    }
    
    // Physics objects - blue
    if (entityManager.HasComponent<RigidbodyComponent>(entityId)) {
        return ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
    }
    
    // Moving objects - cyan
    if (entityManager.HasComponent<VelocityComponent>(entityId)) {
        return ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Material objects - purple
    if (entityManager.HasComponent<MaterialComponent>(entityId)) {
        return ImVec4(0.8f, 0.3f, 0.8f, 1.0f);
    }
    
    // Parent objects - white
    if (HasChildren(entityId)) {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Default - gray
    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
}

void HierarchyPanel::SelectEntity(EntityID entityId, bool ctrlHeld, bool shiftHeld) {
    
    if (shiftHeld && m_lastClickedEntity != INVALID_ENTITY) {
        // Shift-click: range selection
        SelectRange(m_lastClickedEntity, entityId);
    } else if (ctrlHeld) {
        // Ctrl-click: toggle selection
        if (IsEntitySelected(entityId)) {
            m_selectedEntities.erase(entityId);
            if (m_primarySelection == entityId) {
                m_primarySelection = m_selectedEntities.empty() ? INVALID_ENTITY : *m_selectedEntities.begin();
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
    m_primarySelection = INVALID_ENTITY;
    m_lastClickedEntity = INVALID_ENTITY;
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
    // Use cached root entities if available
    if (!m_hierarchyDirty && !m_cachedRootEntities.empty()) {
        return m_cachedRootEntities;
    }
    
    // Otherwise fall back to recalculating
    UpdateCaches();
    return m_cachedRootEntities;
}

std::vector<EntityID> HierarchyPanel::GetChildEntities(EntityID parentId) const {
    // Check cache first
    if (!m_childrenCacheDirty) {
        auto it = m_childrenCache.find(parentId);
        if (it != m_childrenCache.end()) {
            return it->second;
        }
    }
    
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(parentId)) {
        return {};
    }
    
    auto* transform = entityManager.GetComponent<TransformComponent>(parentId);
    if (!transform) {
        return {};
    }
    
    // Cache the result
    m_childrenCache[parentId] = transform->children;
    return transform->children;
}

bool HierarchyPanel::HasChildren(EntityID entityId) const {
    return !GetChildEntities(entityId).empty();
}

void HierarchyPanel::HandleKeyboardInput() {
    if (!ImGui::IsWindowFocused()) return;
    
    // F2 or Enter: rename selected entity
    if ((ImGui::IsKeyPressed(static_cast<ImGuiKey>(291)) || ImGui::IsKeyPressed(static_cast<ImGuiKey>(257))) && // 291 = F2, 257 = Enter
        m_primarySelection != INVALID_ENTITY && m_renamingEntity == INVALID_ENTITY) {
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
    
    if (!entityManager.IsEntityValid(entityId)) return;
    
    auto* nameComponent = entityManager.GetComponent<NameComponent>(entityId);
    if (!nameComponent) {
        // Add name component if it doesn't exist
        NameComponent newNameComp;
        newNameComp.name = newName;
        entityManager.AddComponent<NameComponent>(entityId, std::move(newNameComp));
    } else {
        nameComponent->name = newName;
    }
    
    // Invalidate name cache
    m_cachedDisplayNames.erase(entityId);
    
    std::cout << "Renamed entity " << entityId << " to '" << newName << "'" << std::endl;
}

void HierarchyPanel::DeleteSelectedEntities() {
    auto& entityManager = EntityManager::Instance();
    
    // Record delete operation before deleting
    EntityOperation op;
    op.type = EntityOperation::Delete;
    op.affectedEntities = std::vector<EntityID>(m_selectedEntities.begin(), m_selectedEntities.end());
    // Store names for potential recreation
    if (!m_selectedEntities.empty()) {
        auto* nameComp = entityManager.GetComponent<NameComponent>(*m_selectedEntities.begin());
        if (nameComp) {
            op.oldName = nameComp->name;
        }
    }
    RecordOperation(op);
    
    for (EntityID entityId : m_selectedEntities) {
        std::cout << "Deleting entity " << entityId << std::endl;
        entityManager.DestroyEntity(entityId);
    }
    
    // Invalidate caches
    InvalidateHierarchy();
    InvalidateChildrenCache();
    
    ClearSelection();
}

void HierarchyPanel::DuplicateSelectedEntities() {
    if (m_selectedEntities.empty()) return;
    
    // Copy selected entities to clipboard and immediately paste
    CopySelectedEntities();
    PasteEntities();
}

void HierarchyPanel::CreateChildEntity(EntityID parentId) {
    auto& entityManager = EntityManager::Instance();
    
    // Create new entity
    EntityID newEntityId = entityManager.CreateEntity();
    
    if (!newEntityId.IsValid()) return;
    
    // Add basic components
    NameComponent nameComp;
    nameComp.name = "New Entity";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    // Inherit parent's position if available
    if (parentId != INVALID_ENTITY) {
        auto* parentTransform = entityManager.GetComponent<TransformComponent>(parentId);
        if (parentTransform) {
            transformComp.position = parentTransform->position;
        }
    } else {
        transformComp.position = m_defaultEntityPosition; // Center of world
    }
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    
    auto transformResult = entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    TransformComponent* transform = transformResult ? transformResult.GetValue() : nullptr;
    
    // Add visual components
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    // Set parent relationship if specified
    if (parentId != INVALID_ENTITY && transform) {
        auto* parentTransform = entityManager.GetComponent<TransformComponent>(parentId);
        if (parentTransform) {
            transform->parent = parentId;
            parentTransform->children.push_back(newEntityId);
            
            // Expand the parent node
            m_expandedNodes.insert(parentId);
        }
    }
    
    // Invalidate caches
    InvalidateHierarchy();
    InvalidateChildrenCache();
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
    
    std::cout << "Created new entity " << newEntityId << " with parent " << parentId << std::endl;
}

void HierarchyPanel::CreateEmpty() {
    auto& entityManager = EntityManager::Instance();
    
    // Create new empty entity
    EntityID newEntityId = entityManager.CreateEntity("Empty");
    std::cout << "CreateEmpty: Created entity with ID: " << newEntityId << " (valid: " << newEntityId.IsValid() << ")" << std::endl;
    if (!newEntityId.IsValid()) return;
    
    // Record the create operation for undo
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Empty";
    RecordOperation(op);
    
    // Add basic components
    NameComponent nameComp;
    nameComp.name = "Empty";
    auto nameResult = entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    std::cout << "CreateEmpty: Added NameComponent: " << (nameResult.IsSuccess() ? "SUCCESS" : "FAILED") << std::endl;
    
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition; // Center of world
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    auto transformResult = entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    std::cout << "CreateEmpty: Added TransformComponent: " << (transformResult.IsSuccess() ? "SUCCESS" : "FAILED") << std::endl;
    
    // Add visual components so it appears in the scene
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize; // Default size
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 1; // Default material
    }
    
    // Invalidate caches
    InvalidateHierarchy();
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreateRigidbodyEntity() {
    auto& entityManager = EntityManager::Instance();
    
    // Create new entity with rigidbody
    EntityID newEntityId = entityManager.CreateEntity("Rigidbody");
    
    if (!newEntityId.IsValid()) return;
    
    // Add required components
    NameComponent nameComp;
    nameComp.name = "Rigidbody";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition; // Center of world
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    // Add visual components so it appears in the scene
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize; // Default size
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 2; // Different material for rigidbody objects
    }
    
    // Add rigidbody component with sensible defaults
    RigidbodyComponent rigidbodyComp;
    auto rigidbodyResult = entityManager.AddComponent<RigidbodyComponent>(newEntityId, std::move(rigidbodyComp));
    if (rigidbodyResult) {
        rigidbodyResult.GetValue()->mass = 1.0f;
        rigidbodyResult.GetValue()->useGravity = true;
        rigidbodyResult.GetValue()->drag = 0.0f;
        rigidbodyResult.GetValue()->angularDrag = 0.05f;
    }
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
    
    std::cout << "Created rigidbody entity " << newEntityId << " with physics and visual components" << std::endl;
}

void HierarchyPanel::CreatePointLightEntity() {
    auto& entityManager = EntityManager::Instance();
    
    // Create new entity with point light
    EntityID newEntityId = entityManager.CreateEntity("Point Light");
    
    if (!newEntityId.IsValid()) return;
    
    // Add required components
    NameComponent nameComp;
    nameComp.name = "Point Light";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition; // Center of world
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    // Add visual components (lights should also be visible)
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize * 0.75f; // Smaller size for light icon
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 3; // Light material (could be glowing/bright)
    }
    
    // Add light component with sensible defaults for point light
    LightComponent lightComp;
    lightComp.type = LightComponent::Point;
    lightComp.color = Vector3{1.0f, 1.0f, 1.0f};  // White light
    lightComp.intensity = 1.0f;
    lightComp.range = 10.0f;
    lightComp.enabled = true;
    auto lightResult = entityManager.AddComponent<LightComponent>(newEntityId, std::move(lightComp));
    
    // Select the new entity
    SelectEntity(newEntityId, false, false);
    
    std::cout << "Created point light entity " << newEntityId << " with light and visual components" << std::endl;
}

void HierarchyPanel::CreateDirectionalLightEntity() {
    auto& entityManager = EntityManager::Instance();
    
    EntityID newEntityId = entityManager.CreateEntity("Directional Light");
    if (!newEntityId.IsValid()) return;
    
    // Record operation
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Directional Light";
    RecordOperation(op);
    
    // Add components
    NameComponent nameComp;
    nameComp.name = "Directional Light";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition;
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    // Directional lights don't need position, just rotation
    LightComponent lightComp;
    lightComp.type = LightComponent::Directional;
    lightComp.color = Vector3{1.0f, 0.95f, 0.8f};  // Warm white
    lightComp.intensity = 1.0f;
    lightComp.enabled = true;
    entityManager.AddComponent<LightComponent>(newEntityId, std::move(lightComp));
    
    // Small visual indicator
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize * 0.5f;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 3;
    }
    
    InvalidateHierarchy();
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreateSpotLightEntity() {
    auto& entityManager = EntityManager::Instance();
    
    EntityID newEntityId = entityManager.CreateEntity("Spot Light");
    if (!newEntityId.IsValid()) return;
    
    // Record operation
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Spot Light";
    RecordOperation(op);
    
    // Add components
    NameComponent nameComp;
    nameComp.name = "Spot Light";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition;
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    LightComponent lightComp;
    lightComp.type = LightComponent::Spot;
    lightComp.color = Vector3{1.0f, 1.0f, 1.0f};
    lightComp.intensity = 2.0f;
    lightComp.range = 15.0f;
    lightComp.innerCone = 25.0f;
    lightComp.outerCone = 35.0f;
    lightComp.enabled = true;
    entityManager.AddComponent<LightComponent>(newEntityId, std::move(lightComp));
    
    // Visual indicator
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize * 0.75f;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 3;
    }
    
    InvalidateHierarchy();
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreateAreaLightEntity() {
    auto& entityManager = EntityManager::Instance();
    
    EntityID newEntityId = entityManager.CreateEntity("Area Light");
    if (!newEntityId.IsValid()) return;
    
    // Record operation
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Area Light";
    RecordOperation(op);
    
    // Add components
    NameComponent nameComp;
    nameComp.name = "Area Light";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition;
    transformComp.scale = Vector3(2.0f, 2.0f, 1.0f); // Wider area
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    // Area lights would need special handling in renderer
    LightComponent lightComp;
    lightComp.type = LightComponent::Point; // Fallback to point for now
    lightComp.color = Vector3{1.0f, 1.0f, 1.0f};
    lightComp.intensity = 0.5f;
    lightComp.range = 20.0f;
    lightComp.enabled = true;
    entityManager.AddComponent<LightComponent>(newEntityId, std::move(lightComp));
    
    // Larger visual indicator
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize * 2.0f;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 3;
    }
    
    InvalidateHierarchy();
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreateParticleSystemEntity() {
    auto& entityManager = EntityManager::Instance();
    
    EntityID newEntityId = entityManager.CreateEntity("Particle System");
    if (!newEntityId.IsValid()) return;
    
    // Record operation
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Particle System";
    RecordOperation(op);
    
    // Add components
    NameComponent nameComp;
    nameComp.name = "Particle System";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition;
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    // Visual representation
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 4; // Particle material
    }
    
    // TODO: Add actual ParticleSystemComponent when available
    
    InvalidateHierarchy();
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreateTrailRendererEntity() {
    auto& entityManager = EntityManager::Instance();
    
    EntityID newEntityId = entityManager.CreateEntity("Trail Renderer");
    if (!newEntityId.IsValid()) return;
    
    // Record operation
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Trail Renderer";
    RecordOperation(op);
    
    // Add components
    NameComponent nameComp;
    nameComp.name = "Trail Renderer";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    transformComp.position = m_defaultEntityPosition;
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    
    // Visual representation
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize * 0.5f;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    // Add velocity for trail effect
    VelocityComponent velocityComp;
    velocityComp.velocity = Vector3(0.0f, 0.0f, 0.0f);
    entityManager.AddComponent<VelocityComponent>(newEntityId, std::move(velocityComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 5; // Trail material
    }
    
    // TODO: Add actual TrailRendererComponent when available
    
    InvalidateHierarchy();
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::CreatePointLightChild(EntityID parentId) {
    auto& entityManager = EntityManager::Instance();
    
    EntityID newEntityId = entityManager.CreateEntity("Point Light");
    if (!newEntityId.IsValid()) return;
    
    // Record operation
    EntityOperation op;
    op.type = EntityOperation::Create;
    op.entityId = newEntityId;
    op.newName = "Point Light";
    RecordOperation(op);
    
    // Add components
    NameComponent nameComp;
    nameComp.name = "Point Light";
    entityManager.AddComponent<NameComponent>(newEntityId, std::move(nameComp));
    
    TransformComponent transformComp;
    // Inherit parent's position
    if (parentId != INVALID_ENTITY) {
        auto* parentTransform = entityManager.GetComponent<TransformComponent>(parentId);
        if (parentTransform) {
            transformComp.position = parentTransform->position;
        }
    } else {
        transformComp.position = m_defaultEntityPosition;
    }
    transformComp.scale = Vector3(1.0f, 1.0f, 1.0f);
    
    auto transformResult = entityManager.AddComponent<TransformComponent>(newEntityId, std::move(transformComp));
    TransformComponent* transform = transformResult ? transformResult.GetValue() : nullptr;
    
    // Set parent relationship
    if (parentId != INVALID_ENTITY && transform) {
        auto* parentTransform = entityManager.GetComponent<TransformComponent>(parentId);
        if (parentTransform) {
            transform->parent = parentId;
            parentTransform->children.push_back(newEntityId);
            m_expandedNodes.insert(parentId);
        }
    }
    
    // Add light component
    LightComponent lightComp;
    lightComp.type = LightComponent::Point;
    lightComp.color = Vector3{1.0f, 1.0f, 1.0f};
    lightComp.intensity = 1.0f;
    lightComp.range = 10.0f;
    lightComp.enabled = true;
    entityManager.AddComponent<LightComponent>(newEntityId, std::move(lightComp));
    
    // Visual indicator
    SpriteComponent spriteComp;
    spriteComp.size = m_defaultSpriteSize * 0.75f;
    spriteComp.visible = true;
    entityManager.AddComponent<SpriteComponent>(newEntityId, std::move(spriteComp));
    
    MaterialComponent materialComp;
    auto materialResult = entityManager.AddComponent<MaterialComponent>(newEntityId, std::move(materialComp));
    if (materialResult) {
        materialResult.GetValue()->materialID = 3;
    }
    
    InvalidateHierarchy();
    InvalidateChildrenCache();
    SelectEntity(newEntityId, false, false);
}

void HierarchyPanel::ShowContextMenu() {
    if (ImGui::BeginPopup("HierarchyContextMenu")) {
        if (m_contextMenuEntity != INVALID_ENTITY) {
            std::string entityName = GetEntityDisplayName(m_contextMenuEntity);
            ImGui::Text("Entity: %s", entityName.c_str());
            ImGui::Separator();
            
            // Creation submenu
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Empty Child")) {
                    CreateChildEntity(m_contextMenuEntity);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Light > Point Light")) {
                    CreatePointLightChild(m_contextMenuEntity);
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            // Edit operations
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                CutSelectedEntities();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                CopySelectedEntities();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, HasClipboardData())) {
                PasteEntities();
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
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
            
            // Visibility and locking
            bool isVisible = IsEntityVisible(m_contextMenuEntity);
            bool isLocked = IsEntityLocked(m_contextMenuEntity);
            
            if (ImGui::MenuItem(isVisible ? "Hide" : "Show")) {
                ToggleVisibility(m_contextMenuEntity);
            }
            if (ImGui::MenuItem(isLocked ? "Unlock" : "Lock")) {
                ToggleLock(m_contextMenuEntity);
            }
            
            ImGui::Separator();
            
            // Navigation
            if (ImGui::MenuItem("Focus", "Double-Click")) {
                FocusCameraOnEntity(m_contextMenuEntity);
            }
            
            ImGui::Separator();
            
            // Selection operations
            if (ImGui::MenuItem("Select Children")) {
                SelectChildren(m_contextMenuEntity);
            }
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                SelectAll();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Delete", "Del")) {
                EntityOperation op;
                op.type = EntityOperation::Delete;
                op.entityId = m_contextMenuEntity;
                op.affectedEntities = std::vector<EntityID>(m_selectedEntities.begin(), m_selectedEntities.end());
                RecordOperation(op);
                
                DeleteSelectedEntities();
            }
        } else {
            // Context menu in empty space - object creation menu
            if (ImGui::MenuItem("Create Empty")) {
                CreateEmpty();
            }
            
            if (ImGui::BeginMenu("2D Object")) {
                if (ImGui::MenuItem("Sprite")) { CreateEmpty(); }
                if (ImGui::MenuItem("UI Canvas")) { CreateEmpty(); }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Point Light")) { CreatePointLightEntity(); }
                if (ImGui::MenuItem("Directional Light")) { CreateDirectionalLightEntity(); }
                if (ImGui::MenuItem("Spot Light")) { CreateSpotLightEntity(); }
                if (ImGui::MenuItem("Area Light")) { CreateAreaLightEntity(); }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Effects")) {
                if (ImGui::MenuItem("Particle System")) { CreateParticleSystemEntity(); }
                if (ImGui::MenuItem("Trail Renderer")) { CreateTrailRendererEntity(); }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Physics")) {
                if (ImGui::MenuItem("Rigidbody")) { CreateRigidbodyEntity(); }
                if (ImGui::MenuItem("Box Collider")) { CreateEmpty(); }
                if (ImGui::MenuItem("Sphere Collider")) { CreateEmpty(); }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, HasClipboardData())) {
                PasteEntities();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                SelectAll();
            }
        }
        
        ImGui::EndPopup();
    }
}

void HierarchyPanel::HandleDragDropPayload(EntityID targetEntity, EntityID draggedEntity) {
    auto& entityManager = EntityManager::Instance();
    
    // Prevent dropping entity onto itself or its children
    if (draggedEntity == targetEntity || IsChildOf(targetEntity, draggedEntity)) {
        return;
    }
    
    // Handle multi-selection drag
    std::vector<EntityID> entitiesToMove;
    if (m_selectedEntities.count(draggedEntity) > 0 && m_selectedEntities.size() > 1) {
        // Move all selected entities
        for (EntityID selected : m_selectedEntities) {
            if (!IsChildOf(targetEntity, selected)) {
                entitiesToMove.push_back(selected);
            }
        }
    } else {
        // Move single entity
        entitiesToMove.push_back(draggedEntity);
    }
    
    // Handle different drop positions
    if (m_currentDropPosition == DropPosition::Inside) {
        // Reparent entities as children
        for (EntityID moveEntity : entitiesToMove) {
            auto* transform = entityManager.GetComponent<TransformComponent>(moveEntity);
            if (transform) {
                // Record operation for undo
                EntityOperation op;
                op.type = EntityOperation::Reparent;
                op.entityId = moveEntity;
                op.parentId = transform->parent;
                RecordOperation(op);
                
                // Remove from old parent
                if (transform->parent != INVALID_ENTITY) {
                    auto* oldParentTransform = entityManager.GetComponent<TransformComponent>(transform->parent);
                    if (oldParentTransform) {
                        auto& children = oldParentTransform->children;
                        children.erase(std::remove(children.begin(), children.end(), moveEntity), children.end());
                    }
                }
                
                // Set new parent
                transform->parent = targetEntity;
                
                // Add to new parent's children
                auto* newParentTransform = entityManager.GetComponent<TransformComponent>(targetEntity);
                if (newParentTransform) {
                    newParentTransform->children.push_back(moveEntity);
                }
                
                // Expand the parent to show the new child
                m_expandedNodes.insert(targetEntity);
            }
        }
        
        std::cout << "Reparented " << entitiesToMove.size() << " entities to " << GetEntityDisplayName(targetEntity) << std::endl;
    } else {
        // Handle sibling reordering (Above/Below)
        auto* targetTransform = entityManager.GetComponent<TransformComponent>(targetEntity);
        if (!targetTransform) return;
        
        EntityID parentId = targetTransform->parent;
        auto* parentTransform = parentId != INVALID_ENTITY 
            ? entityManager.GetComponent<TransformComponent>(parentId) 
            : nullptr;
        
        // Get the target's siblings list (either from parent or root entities)
        std::vector<EntityID>* siblingsList = nullptr;
        if (parentTransform) {
            siblingsList = &parentTransform->children;
        } else {
            // Handle root entity reordering
            auto targetIt = std::find(m_rootEntityOrder.begin(), m_rootEntityOrder.end(), targetEntity);
            if (targetIt == m_rootEntityOrder.end()) return;
            
            size_t targetIndex = std::distance(m_rootEntityOrder.begin(), targetIt);
            if (m_currentDropPosition == DropPosition::Below) {
                targetIndex++;
            }
            
            // Process each entity to move
            for (EntityID moveEntity : entitiesToMove) {
                auto* moveTransform = entityManager.GetComponent<TransformComponent>(moveEntity);
                if (!moveTransform) continue;
                
                // Remove from old parent if it has one
                if (moveTransform->parent != INVALID_ENTITY) {
                    auto* oldParentTransform = entityManager.GetComponent<TransformComponent>(moveTransform->parent);
                    if (oldParentTransform) {
                        auto& oldChildren = oldParentTransform->children;
                        oldChildren.erase(std::remove(oldChildren.begin(), oldChildren.end(), moveEntity), oldChildren.end());
                    }
                    moveTransform->parent = INVALID_ENTITY;
                }
                
                // Remove from current position in root order
                auto moveIt = std::find(m_rootEntityOrder.begin(), m_rootEntityOrder.end(), moveEntity);
                if (moveIt != m_rootEntityOrder.end()) {
                    size_t moveIndex = std::distance(m_rootEntityOrder.begin(), moveIt);
                    if (moveIndex < targetIndex) {
                        targetIndex--;
                    }
                    m_rootEntityOrder.erase(moveIt);
                }
                
                // Insert at target position
                if (targetIndex <= m_rootEntityOrder.size()) {
                    m_rootEntityOrder.insert(m_rootEntityOrder.begin() + targetIndex, moveEntity);
                    targetIndex++;
                } else {
                    m_rootEntityOrder.push_back(moveEntity);
                }
            }
            
            std::cout << "Reordered " << entitiesToMove.size() << " root entities" << std::endl;
            return;
        }
        
        // Find target position in siblings
        auto targetIt = std::find(siblingsList->begin(), siblingsList->end(), targetEntity);
        if (targetIt == siblingsList->end()) return;
        
        size_t targetIndex = std::distance(siblingsList->begin(), targetIt);
        if (m_currentDropPosition == DropPosition::Below) {
            targetIndex++;
        }
        
        // Process each entity to move
        for (EntityID moveEntity : entitiesToMove) {
            auto* moveTransform = entityManager.GetComponent<TransformComponent>(moveEntity);
            if (!moveTransform) continue;
            
            // Skip if trying to move to same parent at same position
            if (moveTransform->parent == parentId) {
                auto moveIt = std::find(siblingsList->begin(), siblingsList->end(), moveEntity);
                if (moveIt != siblingsList->end()) {
                    size_t moveIndex = std::distance(siblingsList->begin(), moveIt);
                    if ((m_currentDropPosition == DropPosition::Above && moveIndex == targetIndex - 1) ||
                        (m_currentDropPosition == DropPosition::Below && moveIndex == targetIndex)) {
                        continue;
                    }
                }
            }
            
            // Remove from old parent
            if (moveTransform->parent != INVALID_ENTITY) {
                auto* oldParentTransform = entityManager.GetComponent<TransformComponent>(moveTransform->parent);
                if (oldParentTransform) {
                    auto& oldChildren = oldParentTransform->children;
                    auto oldIt = std::find(oldChildren.begin(), oldChildren.end(), moveEntity);
                    if (oldIt != oldChildren.end()) {
                        // If moving within same parent, adjust target index
                        if (moveTransform->parent == parentId) {
                            size_t oldIndex = std::distance(oldChildren.begin(), oldIt);
                            if (oldIndex < targetIndex) {
                                targetIndex--;
                            }
                        }
                        oldChildren.erase(oldIt);
                    }
                }
            }
            
            // Set new parent
            moveTransform->parent = parentId;
            
            // Insert at target position
            if (targetIndex <= siblingsList->size()) {
                siblingsList->insert(siblingsList->begin() + targetIndex, moveEntity);
                targetIndex++; // Adjust for next entity
            } else {
                siblingsList->push_back(moveEntity);
            }
        }
        
        std::cout << "Reordered " << entitiesToMove.size() << " entities" << std::endl;
    }
    
    // Clear drop target after successful drop
    m_dropTargetEntity = INVALID_ENTITY;
}

void HierarchyPanel::HandleDragAndDrop(EntityID entityId) {
    // This function is deprecated - drag & drop is now handled inline in RenderEntityNode
    (void)entityId;
}

void HierarchyPanel::HandleMaterialDragAndDrop(EntityID entityId) {
    // This function is deprecated - material drag & drop is now handled inline in RenderEntityNode
    (void)entityId;
}

void HierarchyPanel::ApplyMaterialToEntity(EntityID entityId, const std::string& materialPath) {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
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
    auto* materialComponent = entityManager.GetComponent<MaterialComponent>(entityId);
    if (!materialComponent) {
        MaterialComponent materialComp;
        materialComp.materialID = materialID;
        entityManager.AddComponent<MaterialComponent>(entityId, std::move(materialComp));
    } else {
        // Apply the new material
        materialComponent->materialID = materialID;
    }
    
    std::cout << "Applied material " << materialPath << " (ID: " << materialID << ") to entity " << entityId << std::endl;
}

void HierarchyPanel::SelectRange(EntityID fromEntity, EntityID toEntity) {
    // Get all visible entities in order
    std::vector<EntityID> orderedEntities;
    CollectVisibleEntitiesInOrder(orderedEntities);
    
    // Find indices of from and to entities
    auto fromIt = std::find(orderedEntities.begin(), orderedEntities.end(), fromEntity);
    auto toIt = std::find(orderedEntities.begin(), orderedEntities.end(), toEntity);
    
    if (fromIt == orderedEntities.end() || toIt == orderedEntities.end()) {
        return;
    }
    
    // Ensure fromIt comes before toIt
    if (std::distance(orderedEntities.begin(), fromIt) > std::distance(orderedEntities.begin(), toIt)) {
        std::swap(fromIt, toIt);
    }
    
    // Clear current selection and select range
    m_selectedEntities.clear();
    
    for (auto it = fromIt; it <= toIt; ++it) {
        m_selectedEntities.insert(*it);
    }
    
    m_primarySelection = toEntity;
    BroadcastSelectionChanged();
}

std::vector<EntityID> HierarchyPanel::GetEntitiesBetween(EntityID start, EntityID end) const {
    std::vector<EntityID> result;
    std::vector<EntityID> orderedEntities;
    CollectVisibleEntitiesInOrder(const_cast<std::vector<EntityID>&>(orderedEntities));
    
    auto startIt = std::find(orderedEntities.begin(), orderedEntities.end(), start);
    auto endIt = std::find(orderedEntities.begin(), orderedEntities.end(), end);
    
    if (startIt == orderedEntities.end() || endIt == orderedEntities.end()) {
        return result;
    }
    
    if (std::distance(orderedEntities.begin(), startIt) > std::distance(orderedEntities.begin(), endIt)) {
        std::swap(startIt, endIt);
    }
    
    for (auto it = startIt; it <= endIt; ++it) {
        result.push_back(*it);
    }
    
    return result;
}

void HierarchyPanel::CollectVisibleEntitiesInOrder(std::vector<EntityID>& outEntities) const {
    // Collect root entities first
    std::vector<EntityID> rootEntities = GetRootEntities();
    
    for (EntityID root : rootEntities) {
        CollectEntityAndChildrenInOrder(root, outEntities);
    }
}

void HierarchyPanel::CollectEntityAndChildrenInOrder(EntityID entity, std::vector<EntityID>& outEntities) const {
    // Add this entity
    outEntities.push_back(entity);
    
    // If expanded, add children
    if (m_expandedNodes.count(entity) > 0) {
        std::vector<EntityID> children = GetChildEntities(entity);
        for (EntityID child : children) {
            CollectEntityAndChildrenInOrder(child, outEntities);
        }
    }
}

void HierarchyPanel::HandleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Select All (Ctrl+A)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        SelectAll();
    }
    
    // Deselect All (Escape)
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        DeselectAll();
    }
    
    // Copy (Ctrl+C)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
        CopySelectedEntities();
    }
    
    // Cut (Ctrl+X)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X)) {
        CutSelectedEntities();
    }
    
    // Paste (Ctrl+V)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
        PasteEntities();
    }
    
    // Duplicate (Ctrl+D)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
        DuplicateSelectedEntities();
    }
    
    // Delete (Delete key)
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        DeleteSelectedEntities();
    }
    
    // Rename (F2)
    if (ImGui::IsKeyPressed(ImGuiKey_F2) && m_primarySelection != INVALID_ENTITY) {
        m_renamingEntity = m_primarySelection;
        std::string currentName = GetEntityDisplayName(m_primarySelection);
        strncpy(m_renameBuffer, currentName.c_str(), sizeof(m_renameBuffer) - 1);
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }
    
    // Undo (Ctrl+Z)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift) {
        Undo();
    }
    
    // Redo (Ctrl+Y or Ctrl+Shift+Z)
    if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) || 
        (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))) {
        Redo();
    }
    
    // Arrow key navigation
    if (!io.KeyCtrl && !io.KeyShift && !io.KeyAlt) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            NavigateUp();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            NavigateDown();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            ExpandCollapseSelected(false);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            ExpandCollapseSelected(true);
        }
    }
}

void HierarchyPanel::SelectAll() {
    auto& entityManager = EntityManager::Instance();
    
    // Clear current selection
    m_selectedEntities.clear();
    
    // Select all valid entities
    auto allEntities = entityManager.GetAllEntityIDs();
    for (EntityID entity : allEntities) {
        if (entityManager.IsEntityValid(entity)) {
            m_selectedEntities.insert(entity);
        }
    }
    
    // Set primary selection to first entity
    if (!m_selectedEntities.empty()) {
        m_primarySelection = *m_selectedEntities.begin();
    }
    
    BroadcastSelectionChanged();
}

void HierarchyPanel::SelectChildren(EntityID parentEntity) {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(parentEntity)) {
        return;
    }
    
    // Helper function to recursively select all children
    std::function<void(EntityID)> selectChildrenRecursive = [&](EntityID parent) {
        auto children = GetChildEntities(parent);
        for (EntityID child : children) {
            if (entityManager.IsEntityValid(child)) {
                m_selectedEntities.insert(child);
                selectChildrenRecursive(child);  // Recursively select children of children
            }
        }
    };
    
    // Add parent to selection if not already selected
    if (!IsEntitySelected(parentEntity)) {
        m_selectedEntities.insert(parentEntity);
    }
    
    // Select all children recursively
    selectChildrenRecursive(parentEntity);
    
    // Update primary selection
    m_primarySelection = parentEntity;
    
    BroadcastSelectionChanged();
}

void HierarchyPanel::DeselectAll() {
    ClearSelection();
}

void HierarchyPanel::CopySelectedEntities() {
    if (m_selectedEntities.empty()) return;
    
    // Clear clipboard and copy selected entities
    s_clipboard.entities.clear();
    s_clipboard.isCut = false;
    
    for (EntityID entity : m_selectedEntities) {
        s_clipboard.entities.push_back(entity);
    }
    
    std::cout << "Copied " << s_clipboard.entities.size() << " entities to clipboard" << std::endl;
}

void HierarchyPanel::CutSelectedEntities() {
    if (m_selectedEntities.empty()) return;
    
    // Copy entities and mark as cut
    CopySelectedEntities();
    s_clipboard.isCut = true;
    
    std::cout << "Cut " << s_clipboard.entities.size() << " entities to clipboard" << std::endl;
}

void HierarchyPanel::PasteEntities() {
    if (s_clipboard.entities.empty()) return;
    
    auto& entityManager = EntityManager::Instance();
    std::vector<EntityID> pastedEntities;
    
    // Determine parent for pasted entities
    EntityID parentEntity = INVALID_ENTITY;
    if (m_primarySelection != INVALID_ENTITY && entityManager.IsEntityValid(m_primarySelection)) {
        parentEntity = m_primarySelection;
    }
    
    // Clone or move entities
    for (EntityID sourceEntity : s_clipboard.entities) {
        if (!entityManager.IsEntityValid(sourceEntity)) continue;
        
        if (s_clipboard.isCut) {
            // Move entity to new parent
            if (parentEntity != INVALID_ENTITY) {
                auto* transform = entityManager.GetComponent<TransformComponent>(sourceEntity);
                if (transform) {
                    // Remove from old parent
                    if (transform->parent != INVALID_ENTITY) {
                        auto* parentTransform = entityManager.GetComponent<TransformComponent>(transform->parent);
                        if (parentTransform) {
                            auto& children = parentTransform->children;
                            children.erase(std::remove(children.begin(), children.end(), sourceEntity), children.end());
                        }
                    }
                    
                    // Add to new parent
                    transform->parent = parentEntity;
                    auto* newParentTransform = entityManager.GetComponent<TransformComponent>(parentEntity);
                    if (newParentTransform) {
                        newParentTransform->children.push_back(sourceEntity);
                    }
                }
            }
            pastedEntities.push_back(sourceEntity);
        } else {
            // Clone entity
            auto* nameComp = entityManager.GetComponent<NameComponent>(sourceEntity);
            std::string baseName = nameComp ? nameComp->name : "Entity";
            std::string newName = baseName + " (Copy)";
            
            EntityID newEntity = entityManager.CreateEntity(newName);
            
            // Copy all components
            auto* transform = entityManager.GetComponent<TransformComponent>(sourceEntity);
            if (transform) {
                TransformComponent newTransform = *transform;
                newTransform.parent = parentEntity;
                newTransform.children.clear(); // Don't copy children
                entityManager.AddComponent(newEntity, std::move(newTransform));
                
                // Add to parent's children
                if (parentEntity != INVALID_ENTITY) {
                    auto* parentTransform = entityManager.GetComponent<TransformComponent>(parentEntity);
                    if (parentTransform) {
                        parentTransform->children.push_back(newEntity);
                    }
                }
            }
            
            // Copy other components
            if (entityManager.HasComponent<VelocityComponent>(sourceEntity)) {
                auto* comp = entityManager.GetComponent<VelocityComponent>(sourceEntity);
                entityManager.AddComponent(newEntity, VelocityComponent(*comp));
            }
            if (entityManager.HasComponent<MaterialComponent>(sourceEntity)) {
                auto* comp = entityManager.GetComponent<MaterialComponent>(sourceEntity);
                entityManager.AddComponent(newEntity, MaterialComponent(*comp));
            }
            if (entityManager.HasComponent<SpriteComponent>(sourceEntity)) {
                auto* comp = entityManager.GetComponent<SpriteComponent>(sourceEntity);
                entityManager.AddComponent(newEntity, SpriteComponent(*comp));
            }
            if (entityManager.HasComponent<HealthComponent>(sourceEntity)) {
                auto* comp = entityManager.GetComponent<HealthComponent>(sourceEntity);
                entityManager.AddComponent(newEntity, HealthComponent(*comp));
            }
            if (entityManager.HasComponent<LightComponent>(sourceEntity)) {
                auto* comp = entityManager.GetComponent<LightComponent>(sourceEntity);
                entityManager.AddComponent(newEntity, LightComponent(*comp));
            }
            if (entityManager.HasComponent<RigidbodyComponent>(sourceEntity)) {
                auto* comp = entityManager.GetComponent<RigidbodyComponent>(sourceEntity);
                entityManager.AddComponent(newEntity, RigidbodyComponent(*comp));
            }
            
            pastedEntities.push_back(newEntity);
        }
    }
    
    // Clear clipboard if it was a cut operation
    if (s_clipboard.isCut) {
        s_clipboard.entities.clear();
        s_clipboard.isCut = false;
    }
    
    // Select pasted entities
    ClearSelection();
    for (EntityID entity : pastedEntities) {
        m_selectedEntities.insert(entity);
    }
    if (!pastedEntities.empty()) {
        m_primarySelection = pastedEntities.back();
    }
    
    BroadcastSelectionChanged();
    
    std::cout << "Pasted " << pastedEntities.size() << " entities" << std::endl;
}

bool HierarchyPanel::HasClipboardData() const {
    return !s_clipboard.entities.empty();
}

bool HierarchyPanel::IsChildOf(EntityID potentialChild, EntityID potentialParent) const {
    auto& entityManager = EntityManager::Instance();
    
    // Check if potentialChild is a descendant of potentialParent
    auto* childTransform = entityManager.GetComponent<TransformComponent>(potentialChild);
    if (!childTransform) return false;
    
    EntityID currentParent = childTransform->parent;
    while (currentParent != INVALID_ENTITY) {
        if (currentParent == potentialParent) {
            return true;
        }
        
        auto* parentTransform = entityManager.GetComponent<TransformComponent>(currentParent);
        if (!parentTransform) break;
        
        currentParent = parentTransform->parent;
    }
    
    return false;
}

// Search and filtering implementation
void HierarchyPanel::UpdateSearchResults() {
    m_searchResults.clear();
    
    if (m_searchQuery.empty()) {
        m_showOnlySearchResults = false;
        return;
    }
    
    m_showOnlySearchResults = true;
    auto& entityManager = EntityManager::Instance();
    auto allEntities = entityManager.GetAllEntityIDs();
    
    // Convert search query to lowercase for case-insensitive search
    std::string queryLower = m_searchQuery;
    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
    
    for (EntityID entity : allEntities) {
        std::string name = GetEntityDisplayName(entity);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if (name.find(queryLower) != std::string::npos) {
            m_searchResults.insert(entity);
            
            // Also add all parents to ensure the path is visible
            auto* transform = entityManager.GetComponent<TransformComponent>(entity);
            EntityID parent = transform ? transform->parent : INVALID_ENTITY;
            while (parent != INVALID_ENTITY) {
                m_searchResults.insert(parent);
                auto* parentTransform = entityManager.GetComponent<TransformComponent>(parent);
                parent = parentTransform ? parentTransform->parent : INVALID_ENTITY;
            }
        }
    }
}

bool HierarchyPanel::MatchesSearchFilter(EntityID entityId) const {
    if (m_searchQuery.empty()) return false;
    
    std::string name = GetEntityDisplayName(entityId);
    std::string queryLower = m_searchQuery;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
    
    return name.find(queryLower) != std::string::npos;
}

bool HierarchyPanel::IsVisible(EntityID entityId) const {
    if (!m_showOnlySearchResults || m_searchQuery.empty()) {
        return true;
    }
    
    // Check if entity or any of its children match the search
    if (m_searchResults.count(entityId) > 0) {
        return true;
    }
    
    // Check if any child matches
    std::vector<EntityID> children = GetChildEntities(entityId);
    for (EntityID child : children) {
        if (IsVisible(child)) {
            return true;
        }
    }
    
    return false;
}

// Visibility and locking implementation
void HierarchyPanel::ToggleVisibility(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    
    if (m_hiddenEntities.count(entityId) > 0) {
        m_hiddenEntities.erase(entityId);
        
        // Show the entity by enabling its sprite component
        auto* sprite = entityManager.GetComponent<SpriteComponent>(entityId);
        if (sprite) {
            sprite->visible = true;
        }
        
        // Also show all children
        std::vector<EntityID> children = GetChildEntities(entityId);
        for (EntityID child : children) {
            if (m_hiddenEntities.count(child) == 0) {
                auto* childSprite = entityManager.GetComponent<SpriteComponent>(child);
                if (childSprite) {
                    childSprite->visible = true;
                }
            }
        }
    } else {
        m_hiddenEntities.insert(entityId);
        
        // Hide the entity by disabling its sprite component
        auto* sprite = entityManager.GetComponent<SpriteComponent>(entityId);
        if (sprite) {
            sprite->visible = false;
        }
        
        // Also hide all children
        std::vector<EntityID> children = GetChildEntities(entityId);
        for (EntityID child : children) {
            auto* childSprite = entityManager.GetComponent<SpriteComponent>(child);
            if (childSprite) {
                childSprite->visible = false;
            }
        }
    }
    
    // Publish visibility changed event
    if (m_eventBus) {
        EntityVisibilityChangedEvent event(entityId, IsEntityVisible(entityId));
        m_eventBus->Publish(event);
    }
}

void HierarchyPanel::ToggleLock(EntityID entityId) {
    if (m_lockedEntities.count(entityId) > 0) {
        m_lockedEntities.erase(entityId);
    } else {
        m_lockedEntities.insert(entityId);
        
        // Deselect if currently selected
        if (IsEntitySelected(entityId)) {
            m_selectedEntities.erase(entityId);
            if (m_primarySelection == entityId) {
                m_primarySelection = m_selectedEntities.empty() ? INVALID_ENTITY : *m_selectedEntities.begin();
            }
            BroadcastSelectionChanged();
        }
    }
}

bool HierarchyPanel::IsEntityVisible(EntityID entityId) const {
    return m_hiddenEntities.count(entityId) == 0;
}

bool HierarchyPanel::IsEntityLocked(EntityID entityId) const {
    return m_lockedEntities.count(entityId) > 0;
}

// Undo/Redo implementation
void HierarchyPanel::RecordOperation(const EntityOperation& op) {
    m_undoHistory.push_back(op);
    if (m_undoHistory.size() > MAX_UNDO_HISTORY) {
        m_undoHistory.pop_front();
    }
    
    // Clear redo history when new operation is recorded
    m_redoHistory.clear();
}

void HierarchyPanel::Undo() {
    if (m_undoHistory.empty()) return;
    
    EntityOperation op = m_undoHistory.back();
    m_undoHistory.pop_back();
    
    auto& entityManager = EntityManager::Instance();
    
    switch (op.type) {
        case EntityOperation::Create:
            // Undo create by deleting the entity
            if (entityManager.IsEntityValid(op.entityId)) {
                entityManager.DestroyEntity(op.entityId);
            }
            break;
            
        case EntityOperation::Delete:
            // Undo delete by recreating the entities
            // Note: This is simplified - in a real system we'd store component data
            for (EntityID deletedId : op.affectedEntities) {
                EntityID newId = entityManager.CreateEntity(op.oldName);
                // TODO: Restore component data from operation
            }
            break;
            
        case EntityOperation::Rename:
            if (entityManager.IsEntityValid(op.entityId)) {
                auto* nameComp = entityManager.GetComponent<NameComponent>(op.entityId);
                if (nameComp) {
                    nameComp->name = op.oldName;
                }
            }
            break;
            
        case EntityOperation::Reparent:
            if (entityManager.IsEntityValid(op.entityId)) {
                auto* transform = entityManager.GetComponent<TransformComponent>(op.entityId);
                if (transform) {
                    // Remove from current parent
                    if (transform->parent != INVALID_ENTITY) {
                        auto* currentParent = entityManager.GetComponent<TransformComponent>(transform->parent);
                        if (currentParent) {
                            auto& children = currentParent->children;
                            children.erase(std::remove(children.begin(), children.end(), op.entityId), children.end());
                        }
                    }
                    
                    // Restore old parent
                    transform->parent = op.parentId;
                    if (op.parentId != INVALID_ENTITY) {
                        auto* oldParent = entityManager.GetComponent<TransformComponent>(op.parentId);
                        if (oldParent) {
                            oldParent->children.push_back(op.entityId);
                        }
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    m_redoHistory.push_back(op);
}

void HierarchyPanel::Redo() {
    if (m_redoHistory.empty()) return;
    
    EntityOperation op = m_redoHistory.back();
    m_redoHistory.pop_back();
    
    auto& entityManager = EntityManager::Instance();
    
    switch (op.type) {
        case EntityOperation::Create:
            // Redo create by recreating the entity
            // Note: This is simplified - in a real system we'd restore with same ID
            {
                EntityID newId = entityManager.CreateEntity(op.newName);
                // TODO: Restore component data from operation
            }
            break;
            
        case EntityOperation::Delete:
            // Redo delete by deleting the entities again
            for (EntityID entityId : op.affectedEntities) {
                if (entityManager.IsEntityValid(entityId)) {
                    entityManager.DestroyEntity(entityId);
                }
            }
            break;
            
        case EntityOperation::Rename:
            if (entityManager.IsEntityValid(op.entityId)) {
                auto* nameComp = entityManager.GetComponent<NameComponent>(op.entityId);
                if (nameComp) {
                    nameComp->name = op.newName;
                }
            }
            break;
            
        case EntityOperation::Reparent:
            // Redo reparent - this logic is complex because we need to find the new parent
            // In a real implementation, we'd store both old and new parent IDs
            if (entityManager.IsEntityValid(op.entityId)) {
                // For now, just log that this needs implementation
                std::cout << "Redo reparent not fully implemented" << std::endl;
            }
            break;
            
        default:
            break;
    }
    
    m_undoHistory.push_back(op);
}

bool HierarchyPanel::CanUndo() const {
    return !m_undoHistory.empty();
}

bool HierarchyPanel::CanRedo() const {
    return !m_redoHistory.empty();
}

// Enhanced drag & drop implementation
HierarchyPanel::DropPosition HierarchyPanel::GetDropPosition(const ImVec2& mousePos, const ImRect& itemRect) const {
    float itemHeight = itemRect.Max.y - itemRect.Min.y;
    float relativeY = mousePos.y - itemRect.Min.y;
    float threshold = itemHeight * m_dropZoneThreshold; // threshold from top/bottom for above/below
    
    if (relativeY < threshold) {
        return DropPosition::Above;
    } else if (relativeY > itemHeight - threshold) {
        return DropPosition::Below;
    } else {
        return DropPosition::Inside;
    }
}

void HierarchyPanel::DrawDropIndicator(DropPosition pos, const ImRect& itemRect) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 lineColor = ImGui::GetColorU32(ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
    float thickness = 3.0f;
    
    switch (pos) {
        case DropPosition::Above:
            // Draw a thick line above the item
            drawList->AddLine(
                ImVec2(itemRect.Min.x + 10, itemRect.Min.y - 1),
                ImVec2(itemRect.Max.x - 10, itemRect.Min.y - 1),
                lineColor, thickness
            );
            // Add small arrows on the sides
            drawList->AddTriangleFilled(
                ImVec2(itemRect.Min.x + 5, itemRect.Min.y - 1),
                ImVec2(itemRect.Min.x + 10, itemRect.Min.y - 4),
                ImVec2(itemRect.Min.x + 10, itemRect.Min.y + 2),
                lineColor
            );
            break;
        
        case DropPosition::Below:
            // Draw a thick line below the item
            drawList->AddLine(
                ImVec2(itemRect.Min.x + 10, itemRect.Max.y + 1),
                ImVec2(itemRect.Max.x - 10, itemRect.Max.y + 1),
                lineColor, thickness
            );
            // Add small arrows on the sides
            drawList->AddTriangleFilled(
                ImVec2(itemRect.Min.x + 5, itemRect.Max.y + 1),
                ImVec2(itemRect.Min.x + 10, itemRect.Max.y - 2),
                ImVec2(itemRect.Min.x + 10, itemRect.Max.y + 4),
                lineColor
            );
            break;
        
        case DropPosition::Inside:
            // Draw a subtle outline only, no fill
            ImU32 insideColor = ImGui::GetColorU32(ImVec4(0.4f, 0.7f, 1.0f, 0.8f));
            drawList->AddRect(
                ImVec2(itemRect.Min.x + 2, itemRect.Min.y + 1),
                ImVec2(itemRect.Max.x - 2, itemRect.Max.y - 1),
                insideColor, 2.0f, 0, 2.0f
            );
            // Add small corner indicators
            float cornerSize = 6.0f;
            // Top-left corner
            drawList->AddLine(
                ImVec2(itemRect.Min.x + 2, itemRect.Min.y + 1 + cornerSize),
                ImVec2(itemRect.Min.x + 2, itemRect.Min.y + 1),
                insideColor, 3.0f
            );
            drawList->AddLine(
                ImVec2(itemRect.Min.x + 2, itemRect.Min.y + 1),
                ImVec2(itemRect.Min.x + 2 + cornerSize, itemRect.Min.y + 1),
                insideColor, 3.0f
            );
            // Bottom-right corner
            drawList->AddLine(
                ImVec2(itemRect.Max.x - 2, itemRect.Max.y - 1 - cornerSize),
                ImVec2(itemRect.Max.x - 2, itemRect.Max.y - 1),
                insideColor, 3.0f
            );
            drawList->AddLine(
                ImVec2(itemRect.Max.x - 2, itemRect.Max.y - 1),
                ImVec2(itemRect.Max.x - 2 - cornerSize, itemRect.Max.y - 1),
                insideColor, 3.0f
            );
            break;
    }
}

// Arrow key navigation implementation
void HierarchyPanel::NavigateUp() {
    if (m_primarySelection == INVALID_ENTITY) return;
    
    EntityID prev = GetNextVisibleEntity(m_primarySelection, false);
    if (prev != INVALID_ENTITY) {
        SelectEntity(prev, false, false);
    }
}

void HierarchyPanel::NavigateDown() {
    if (m_primarySelection == INVALID_ENTITY) return;
    
    EntityID next = GetNextVisibleEntity(m_primarySelection, true);
    if (next != INVALID_ENTITY) {
        SelectEntity(next, false, false);
    }
}

void HierarchyPanel::ExpandCollapseSelected(bool expand) {
    if (m_primarySelection == INVALID_ENTITY) return;
    
    if (expand) {
        m_expandedNodes.insert(m_primarySelection);
    } else {
        m_expandedNodes.erase(m_primarySelection);
    }
}

EntityID HierarchyPanel::GetNextVisibleEntity(EntityID current, bool forward) const {
    // TODO: Implement proper tree traversal for navigation
    // This is a simplified version
    auto& entityManager = EntityManager::Instance();
    auto allEntities = entityManager.GetAllEntityIDs();
    
    auto it = std::find(allEntities.begin(), allEntities.end(), current);
    if (it == allEntities.end()) return INVALID_ENTITY;
    
    if (forward) {
        ++it;
        if (it != allEntities.end()) return *it;
    } else {
        if (it != allEntities.begin()) {
            --it;
            return *it;
        }
    }
    
    return INVALID_ENTITY;
}

void HierarchyPanel::FocusCameraOnEntity(EntityID entityId) {
    auto& entityManager = EntityManager::Instance();
    
    if (!entityManager.IsEntityValid(entityId)) {
        return;
    }
    
    // Get the entity's transform
    auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
    if (!transform) {
        return;
    }
    
    // Publish an event to focus the scene camera on this entity
    if (m_eventBus) {
        FocusCameraEvent event(transform->position, entityId);
        m_eventBus->Publish(event);
    }
    
    std::cout << "Focus camera on entity " << entityId << " at position (" 
              << transform->position.x << ", " << transform->position.y << ", " 
              << transform->position.z << ")" << std::endl;
}

void HierarchyPanel::UpdateCaches() const {
    // Clear all caches
    m_childrenCache.clear();
    m_cachedDisplayNames.clear();
    m_cachedIcons.clear();
    m_cachedRootEntities.clear();
    
    auto& entityManager = EntityManager::Instance();
    auto allEntities = entityManager.GetAllEntityIDs();
    
    // Pre-cache root entities
    for (EntityID entityId : allEntities) {
        if (!entityManager.IsEntityValid(entityId)) {
            continue;
        }
        
        auto* transform = entityManager.GetComponent<TransformComponent>(entityId);
        if (!transform || transform->parent == INVALID_ENTITY) {
            m_cachedRootEntities.push_back(entityId);
        }
    }
    
    // Sort root entities by their order
    std::sort(m_cachedRootEntities.begin(), m_cachedRootEntities.end(),
        [this](EntityID a, EntityID b) {
            auto itA = std::find(m_rootEntityOrder.begin(), m_rootEntityOrder.end(), a);
            auto itB = std::find(m_rootEntityOrder.begin(), m_rootEntityOrder.end(), b);
            
            if (itA != m_rootEntityOrder.end() && itB != m_rootEntityOrder.end()) {
                return std::distance(m_rootEntityOrder.begin(), itA) < 
                       std::distance(m_rootEntityOrder.begin(), itB);
            }
            return a < b;
        });
    
    m_childrenCacheDirty = false;
    m_hierarchyDirty = false;
}

} // namespace BGE