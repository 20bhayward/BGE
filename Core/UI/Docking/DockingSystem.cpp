#include "DockingSystem.h"
#include "LayoutSerializer.h"
#include "LayoutData.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <functional>
#include <filesystem>
#include <fstream>

namespace BGE {

// Constants for docking system
static constexpr float TAB_HEIGHT = 24.0f;

DockingSystem::DockingSystem() {
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
}

void DockingSystem::Initialize() {
    // Start with Unity layout as default
    LoadUnityLayout();
    
    // Load existing layouts from disk
    std::string layoutDir = "layouts/";
    if (std::filesystem::exists(layoutDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(layoutDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string layoutName = entry.path().stem().string();
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    file.close();
                    
                    PersistentLayoutInfo layoutInfo;
                    if (layoutInfo.Deserialize(content)) {
                        m_persistentLayouts[layoutName] = layoutInfo;
                        std::cout << "Loaded saved layout: " << layoutName << std::endl;
                    }
                }
            }
        }
    }
}

void DockingSystem::Shutdown() {
    m_allPanels.clear();
    m_floatingNodes.clear();
    m_rootNode.reset();
    m_dragContext.Reset();
    m_resizeContext.Reset();
    m_dropZones.clear();
}

void DockingSystem::CreateDefaultLayout() {
    if (!m_rootNode) return;
    
    // Get viewport size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) return;
    
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    
    // Set root node size
    m_rootNode->GetData().position = workPos;
    m_rootNode->GetData().size = workSize;
    
    std::cout << "Creating Unity-style default layout..." << std::endl;
    
    // Create Unity-style layout: Left | (Game Center | Right Inspector) | Bottom
    // Root -> Vertical split (Top | Bottom)
    //   Top -> Horizontal split (Left | Center+Right)
    //     Left -> Leaf (Hierarchy)
    //     Center+Right -> Horizontal split (Game | Inspector)
    //       Game -> Leaf (Game viewport - MAIN FOCUS)
    //       Inspector -> Leaf (Inspector, Asset Browser tabs)
    //   Bottom -> Leaf (Materials, Console)
    
    m_rootNode->SetType(DockNodeType::Split);
    m_rootNode->GetData().isHorizontalSplit = false; // Vertical split (top/bottom)
    m_rootNode->GetData().splitRatio = 0.8f; // 80% for top, 20% for bottom
    
    // Create top section
    auto topNode = std::make_shared<DockNode>(DockNodeType::Split);
    topNode->GetData().isHorizontalSplit = true; // Horizontal split (left/right)
    topNode->GetData().splitRatio = 0.2f; // 20% for left hierarchy, 80% for game+inspector
    m_rootNode->AddChild(topNode);
    
    // Create bottom panel (materials)
    auto bottomNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    m_rootNode->AddChild(bottomNode);
    
    // Create left panel (hierarchy)
    auto leftNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    topNode->AddChild(leftNode);
    
    // Create game+inspector split
    auto gameInspectorNode = std::make_shared<DockNode>(DockNodeType::Split);
    gameInspectorNode->GetData().isHorizontalSplit = true; // Horizontal split
    gameInspectorNode->GetData().splitRatio = 0.75f; // 75% for game, 25% for inspector
    topNode->AddChild(gameInspectorNode);
    
    // Create game panel (main focus - largest area)
    auto gameNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    gameInspectorNode->AddChild(gameNode);
    
    // Create inspector panel
    auto inspectorNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    gameInspectorNode->AddChild(inspectorNode);
    
    // Calculate initial layout
    CalculateNodeLayout(m_rootNode, workPos, workSize);
    
    std::cout << "Default layout created with Game panel as main focus!" << std::endl;
}

void DockingSystem::Render() {
    if (!m_rootNode) return;
    
    // Global check: stop any resize operation if mouse is released
    if (m_resizeContext.isResizing && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        std::cout << "Global resize stop - mouse released" << std::endl;
        m_resizeContext.Reset();
    }
    
    
    // Calculate layout for current viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport) {
        CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
    }
    
    // Render main docking area
    RenderNode(m_rootNode);
    
    // Render floating windows
    RenderFloatingNodes();
    
    // Update drag and drop (safe version)
    UpdateDragAndDropSafe();
    
    // Render save layout dialog
    RenderSaveLayoutDialog();
    
    // Cleanup empty nodes
    CleanupEmptyNodes();
}

void DockingSystem::RenderNode(std::shared_ptr<DockNode> node) {
    if (!node) return;
    
    switch (node->GetType()) {
        case DockNodeType::Leaf:
            RenderLeafNode(node);
            break;
        case DockNodeType::Split:
        case DockNodeType::Root:
            RenderSplitNode(node);
            break;
        case DockNodeType::Floating:
            // Floating nodes are rendered separately
            break;
    }
}

void DockingSystem::RenderLeafNode(std::shared_ptr<DockNode> node) {
    if (!node || node->IsEmpty()) return;
    
    const auto& data = node->GetData();
    const auto& panels = node->GetPanels();
    
    // Set next window position and size
    ImGui::SetNextWindowPos(data.position);
    ImGui::SetNextWindowSize(data.size);
    
    // Create a unique window name for this dock node
    std::string windowName = "DockNode_" + node->GetId();
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | 
                           ImGuiWindowFlags_NoMove | 
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoNavFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing;
    
    if (ImGui::Begin(windowName.c_str(), nullptr, flags)) {
        // Render tab bar if multiple panels
        if (panels.size() > 1) {
            RenderTabBar(node);
        } else if (panels.size() == 1) {
            // For single panels, create a draggable header with close button
            auto activePanel = panels[0];
            if (activePanel) {
                // Create tab bar for consistency with multi-tab look
                std::string tabBarId = "SingleTabBar_" + node->GetId();
                if (ImGui::BeginTabBar(tabBarId.c_str(), ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
                    bool isOpen = true;
                    if (ImGui::BeginTabItem(activePanel->GetName().c_str(), &isOpen)) {
                        ImGui::EndTabItem();
                    }
                    
                    // Safe single panel drag detection - get mouse pos within window context
                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) && 
                        !m_dragContext.isDragging) {
                        // Get mouse position safely within window context
                        ImVec2 mousePos = ImGui::GetMousePos();
                        StartDragSafe(activePanel, node, mousePos);
                    }
                    
                    // Check for drag end within window context
                    if (m_dragContext.isDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        EndDragSafe();
                    }
                    
                    // Handle tab close
                    if (!isOpen) {
                        RemovePanel(activePanel);
                    }
                    
                    ImGui::EndTabBar();
                }
            }
        }
        
        // Render active panel content
        auto activePanel = node->GetActivePanel();
        if (activePanel && activePanel->IsVisible()) {
            // All panels should use child windows to ensure proper ImGui context
            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            if (panels.size() > 1) {
                contentSize.y -= TAB_HEIGHT; // Account for tab bar
            }
            
            if (ImGui::BeginChild(("PanelContent_" + activePanel->GetName()).c_str(), 
                                contentSize, false, ImGuiWindowFlags_NoScrollbar)) {
                activePanel->OnRender();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void DockingSystem::RenderSplitNode(std::shared_ptr<DockNode> node) {
    if (!node) return;
    
    const auto& children = node->GetChildren();
    if (children.size() != 2) {
        // Invalid split node, render children anyway
        for (auto& child : children) {
            RenderNode(child);
        }
        return;
    }
    
    // Render both children
    RenderNode(children[0]);
    RenderNode(children[1]);
    
    // Render resize handle between children
    RenderResizeHandles(node);  // Re-enabled with HandleSplitResize disabled
}

void DockingSystem::RenderTabBar(std::shared_ptr<DockNode> node) {
    if (!node || node->GetPanels().empty()) return;
    
    const auto& panels = node->GetPanels();
    std::string tabBarId = "TabBar_" + node->GetId();
    
    if (ImGui::BeginTabBar(tabBarId.c_str(), ImGuiTabBarFlags_Reorderable | 
                                            ImGuiTabBarFlags_AutoSelectNewTabs |
                                            ImGuiTabBarFlags_FittingPolicyScroll)) {
        
        for (int i = 0; i < static_cast<int>(panels.size()); ++i) {
            auto& panel = panels[i];
            if (!panel) continue;
            
            bool isOpen = true;
            ImGuiTabItemFlags flags = 0;
            
            // Check if this tab is being dragged
            bool isBeingDragged = IsTabDragging(node, i);
            if (isBeingDragged) {
                flags |= ImGuiTabItemFlags_NoCloseButton;
            }
            
            if (ImGui::BeginTabItem(panel->GetName().c_str(), &isOpen, flags)) {
                // Set this as the active tab
                if (node->GetActiveTabIndex() != i) {
                    node->SetActiveTab(i);
                }
                
                ImGui::EndTabItem();
            }
            
            // Safe tab drag detection - get mouse pos within window context
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) && 
                !m_dragContext.isDragging) {
                // Get mouse position safely within window context
                ImVec2 mousePos = ImGui::GetMousePos();
                StartDragSafe(panel, node, mousePos);
            }
            
            // Check for drag end within window context
            if (m_dragContext.isDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                EndDragSafe();
            }
            
            // Handle tab close
            if (!isOpen) {
                RemovePanel(panel);
            }
        }
        
        ImGui::EndTabBar();
    }
}

bool DockingSystem::IsTabDragging(std::shared_ptr<DockNode> node, int tabIndex) {
    return m_dragContext.isDragging && 
           m_dragContext.sourceNode == node && 
           node->GetActiveTabIndex() == tabIndex;
}

void DockingSystem::UpdateDragAndDrop() {
    if (!m_dragContext.isDragging) return;
    
    UpdateDrag();
    
    // Check for drag end
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        EndDrag();
    }
    
    // Update drop zones if still dragging
    if (m_dragContext.isDragging) {
        UpdateDropZones();
    }
}

// Safe version that doesn't call problematic ImGui functions outside window context
void DockingSystem::UpdateDragAndDropSafe() {
    if (!m_dragContext.isDragging) return;
    
    // Check for drag end within safe context
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        EndDragSafe();
        return;
    }
    
    // Only update drop zones and render them - don't check mouse state globally
    UpdateDropZones();
    RenderDropZonesSafe();
}

void DockingSystem::StartDrag(std::shared_ptr<Panel> panel, std::shared_ptr<DockNode> sourceNode) {
    if (!panel || !sourceNode) return;
    
    m_dragContext.isDragging = true;
    m_dragContext.draggedPanel = panel;
    m_dragContext.sourceNode = sourceNode;
    m_dragContext.dragStartPos = ImGui::GetMousePos(); // This causes Debug window!
    m_dragContext.dragOffset = {0, 0};
    
    std::cout << "🎯 Started dragging panel: " << panel->GetName() << " - Move mouse to see drop zones!" << std::endl;
}

void DockingSystem::StartDragSafe(std::shared_ptr<Panel> panel, std::shared_ptr<DockNode> sourceNode, ImVec2 mousePos) {
    if (!panel || !sourceNode) return;
    
    m_dragContext.isDragging = true;
    m_dragContext.draggedPanel = panel;
    m_dragContext.sourceNode = sourceNode;
    m_dragContext.dragStartPos = mousePos; // Safe - mouse pos passed from window context
    m_dragContext.dragOffset = {0, 0};
    
    std::cout << "🎯 Started dragging panel: " << panel->GetName() << " - Move mouse to see drop zones!" << std::endl;
}

void DockingSystem::UpdateDrag() {
    if (!m_dragContext.isDragging) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Update drag visualization (could draw the panel being dragged)
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (drawList && m_dragContext.draggedPanel) {
        ImVec2 rectMin = {mousePos.x - 50, mousePos.y - 10};
        ImVec2 rectMax = {mousePos.x + 50, mousePos.y + 10};
        
        drawList->AddRectFilled(rectMin, rectMax, IM_COL32(100, 150, 200, 180));
        drawList->AddRect(rectMin, rectMax, IM_COL32(100, 150, 200, 255));
        drawList->AddText({rectMin.x + 5, rectMin.y + 2}, IM_COL32(255, 255, 255, 255), 
                         m_dragContext.draggedPanel->GetName().c_str());
    }
}

void DockingSystem::EndDrag() {
    if (!m_dragContext.isDragging) return;
    
    std::cout << "EndDrag called" << std::endl;
    
    // Check if we're dropping on a valid drop zone
    DropZone* dropZone = GetHoveredDropZone();
    if (dropZone && dropZone->targetNode && m_dragContext.draggedPanel) {
        std::cout << "Dropping panel " << m_dragContext.draggedPanel->GetName() 
                  << " onto node " << dropZone->targetNode->GetId() 
                  << " in direction " << static_cast<int>(dropZone->direction) << std::endl;
        
        // Remove panel from source
        if (m_dragContext.sourceNode) {
            m_dragContext.sourceNode->RemovePanel(m_dragContext.draggedPanel);
        }
        
        // Dock the panel
        DockPanel(m_dragContext.draggedPanel, dropZone->targetNode, dropZone->direction);
    } else {
        std::cout << "No valid drop zone found - drag cancelled" << std::endl;
    }
    
    // Always reset drag context and clear drop zones
    m_dragContext.Reset();
    m_dropZones.clear();
    
    std::cout << "Drag context reset" << std::endl;
}

// Safe version that checks drop zones using stored mouse position
void DockingSystem::EndDragSafe() {
    if (!m_dragContext.isDragging) return;
    
    std::cout << "EndDragSafe called" << std::endl;
    
    // Get current mouse position safely within window context
    ImVec2 currentMousePos = ImGui::GetMousePos();
    
    // Check if we're dropping on a valid drop zone using current position
    DropZone* dropZone = GetHoveredDropZoneSafe(currentMousePos);
    if (dropZone && dropZone->targetNode && m_dragContext.draggedPanel) {
        std::cout << "Dropping panel " << m_dragContext.draggedPanel->GetName() 
                  << " onto node " << dropZone->targetNode->GetId() 
                  << " in direction " << static_cast<int>(dropZone->direction) << std::endl;
        
        // Remove panel from source
        if (m_dragContext.sourceNode) {
            m_dragContext.sourceNode->RemovePanel(m_dragContext.draggedPanel);
        }
        
        // Dock the panel
        DockPanel(m_dragContext.draggedPanel, dropZone->targetNode, dropZone->direction);
    } else {
        std::cout << "No valid drop zone found - drag cancelled" << std::endl;
    }
    
    // Always reset drag context and clear drop zones
    m_dragContext.Reset();
    m_dropZones.clear();
    
    std::cout << "Drag context reset" << std::endl;
}

void DockingSystem::UpdateDropZones() {
    m_dropZones.clear();
    
    if (m_rootNode) {
        CalculateDropZones(m_rootNode);
    }
    
    // Calculate drop zones for floating windows
    for (auto& floatingNode : m_floatingNodes) {
        if (floatingNode) {
            CalculateDropZones(floatingNode);
        }
    }
}

void DockingSystem::CalculateDropZones(std::shared_ptr<DockNode> node) {
    if (!node) return;
    
    if (node->IsLeaf() && !node->IsEmpty()) {
        const auto& data = node->GetData();
        
        // Create drop zones around this node
        float zoneSize = DROP_ZONE_SIZE;
        
        // Center drop zone (for tabbing)
        DropZone centerZone;
        centerZone.minPos = {data.position.x + zoneSize, data.position.y + zoneSize};
        centerZone.maxPos = {data.position.x + data.size.x - zoneSize, data.position.y + data.size.y - zoneSize};
        centerZone.direction = DockDirection::Center;
        centerZone.targetNode = node;
        m_dropZones.push_back(centerZone);
        
        // Edge drop zones (for splitting)
        
        // Left
        DropZone leftZone;
        leftZone.minPos = {data.position.x, data.position.y};
        leftZone.maxPos = {data.position.x + zoneSize, data.position.y + data.size.y};
        leftZone.direction = DockDirection::Left;
        leftZone.targetNode = node;
        m_dropZones.push_back(leftZone);
        
        // Right
        DropZone rightZone;
        rightZone.minPos = {data.position.x + data.size.x - zoneSize, data.position.y};
        rightZone.maxPos = {data.position.x + data.size.x, data.position.y + data.size.y};
        rightZone.direction = DockDirection::Right;
        rightZone.targetNode = node;
        m_dropZones.push_back(rightZone);
        
        // Top
        DropZone topZone;
        topZone.minPos = {data.position.x, data.position.y};
        topZone.maxPos = {data.position.x + data.size.x, data.position.y + zoneSize};
        topZone.direction = DockDirection::Top;
        topZone.targetNode = node;
        m_dropZones.push_back(topZone);
        
        // Bottom
        DropZone bottomZone;
        bottomZone.minPos = {data.position.x, data.position.y + data.size.y - zoneSize};
        bottomZone.maxPos = {data.position.x + data.size.x, data.position.y + data.size.y};
        bottomZone.direction = DockDirection::Bottom;
        bottomZone.targetNode = node;
        m_dropZones.push_back(bottomZone);
    }
    
    // Recursively calculate for children
    for (auto& child : node->GetChildren()) {
        CalculateDropZones(child);
    }
}

void DockingSystem::RenderDropZones() {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    
    for (auto& zone : m_dropZones) {
        bool isHovered = zone.Contains(mousePos);
        
        ImU32 color = isHovered ? DROP_ZONE_COLOR : IM_COL32(70, 130, 200, 50);
        ImU32 borderColor = isHovered ? DROP_ZONE_BORDER_COLOR : IM_COL32(70, 130, 200, 100);
        
        drawList->AddRectFilled(zone.minPos, zone.maxPos, color);
        drawList->AddRect(zone.minPos, zone.maxPos, borderColor, 0.0f, 0, 2.0f);
        
        zone.isHighlighted = isHovered;
    }
}

// Safe version - show drop zones with highlighting within window context
void DockingSystem::RenderDropZonesSafe() {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;
    
    // Create an invisible window to get mouse position safely
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                           ImGuiWindowFlags_NoResize | 
                           ImGuiWindowFlags_NoMove | 
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBackground |
                           ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoNavFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_NoInputs;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    
    if (ImGui::Begin("DropZoneDetection", nullptr, flags)) {
        // Now we can safely get mouse position within window context
        ImVec2 mousePos = ImGui::GetMousePos();
        
        for (auto& zone : m_dropZones) {
            bool isHovered = zone.Contains(mousePos);
            
            ImU32 color = isHovered ? DROP_ZONE_COLOR : IM_COL32(70, 130, 200, 50);
            ImU32 borderColor = isHovered ? DROP_ZONE_BORDER_COLOR : IM_COL32(70, 130, 200, 100);
            
            drawList->AddRectFilled(zone.minPos, zone.maxPos, color);
            drawList->AddRect(zone.minPos, zone.maxPos, borderColor, 0.0f, 0, 2.0f);
            
            zone.isHighlighted = isHovered;
        }
    }
    ImGui::End();
}

DropZone* DockingSystem::GetHoveredDropZone() {
    ImVec2 mousePos = ImGui::GetMousePos();
    
    for (auto& zone : m_dropZones) {
        if (zone.Contains(mousePos)) {
            return &zone;
        }
    }
    
    return nullptr;
}

// Safe version that takes mouse position as parameter
DropZone* DockingSystem::GetHoveredDropZoneSafe(ImVec2 mousePos) {
    for (auto& zone : m_dropZones) {
        if (zone.Contains(mousePos)) {
            return &zone;
        }
    }
    
    return nullptr;
}

void DockingSystem::DockPanel(std::shared_ptr<Panel> panel, 
                             std::shared_ptr<DockNode> targetNode, 
                             DockDirection direction) {
    if (!panel || !targetNode) return;
    
    if (direction == DockDirection::Center) {
        // Add as tab to existing node
        targetNode->AddPanel(panel);
    } else {
        // Split the target node
        targetNode->Split(direction, panel);
        
        // Recalculate layout
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (viewport && m_rootNode) {
            CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
        }
    }
}

void DockingSystem::CalculateNodeLayout(std::shared_ptr<DockNode> node, ImVec2 position, ImVec2 size) {
    if (!node) return;
    
    auto& data = node->GetData();
    data.position = position;
    data.size = size;
    
    if (node->IsSplit() && node->GetChildren().size() == 2) {
        auto& children = node->GetChildren();
        float splitRatio = data.splitRatio;
        
        if (data.isHorizontalSplit) {
            // Left/Right split
            float leftWidth = size.x * splitRatio;
            float rightWidth = size.x - leftWidth;
            
            CalculateNodeLayout(children[0], position, {leftWidth, size.y});
            CalculateNodeLayout(children[1], {position.x + leftWidth, position.y}, {rightWidth, size.y});
        } else {
            // Top/Bottom split
            float topHeight = size.y * splitRatio;
            float bottomHeight = size.y - topHeight;
            
            CalculateNodeLayout(children[0], position, {size.x, topHeight});
            CalculateNodeLayout(children[1], {position.x, position.y + topHeight}, {size.x, bottomHeight});
        }
    } else {
        // Leaf node or invalid split - just propagate to children
        for (auto& child : node->GetChildren()) {
            CalculateNodeLayout(child, position, size);
        }
    }
}

void DockingSystem::RenderResizeHandles(std::shared_ptr<DockNode> node) {
    if (!node || !node->IsSplit() || node->GetChildren().size() != 2) return;
    
    HandleSplitResize(node);
}

bool DockingSystem::HandleSplitResize(std::shared_ptr<DockNode> node) {
    if (!node || !node->IsSplit() || node->GetChildren().size() != 2) return false;
    
    const auto& data = node->GetData();
    
    ImVec2 handlePos, handleSize;
    ImGuiMouseCursor cursor;
    
    if (data.isHorizontalSplit) {
        // Vertical handle between left and right
        float splitX = data.position.x + data.size.x * data.splitRatio;
        handlePos = {splitX - RESIZE_HANDLE_SIZE * 0.5f, data.position.y};
        handleSize = {RESIZE_HANDLE_SIZE, data.size.y};
        cursor = ImGuiMouseCursor_ResizeEW;
    } else {
        // Horizontal handle between top and bottom
        float splitY = data.position.y + data.size.y * data.splitRatio;
        handlePos = {data.position.x, splitY - RESIZE_HANDLE_SIZE * 0.5f};
        handleSize = {data.size.x, RESIZE_HANDLE_SIZE};
        cursor = ImGuiMouseCursor_ResizeNS;
    }
    
    // Create an invisible window to provide safe ImGui context for mouse detection
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                           ImGuiWindowFlags_NoResize | 
                           ImGuiWindowFlags_NoMove | 
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBackground |
                           ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoNavFocus |
                           ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_NoInputs;
    
    std::string handleWindowName = "ResizeDetection_" + node->GetId();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    
    bool resizeHandled = false;
    
    if (ImGui::Begin(handleWindowName.c_str(), nullptr, flags)) {
        // Now we can safely get mouse position within window context
        ImVec2 mousePos = ImGui::GetMousePos();
        
        ImVec2 rectMin = handlePos;
        ImVec2 rectMax = {handlePos.x + handleSize.x, handlePos.y + handleSize.y};
        
        // Check if this handle is currently being resized
        bool isCurrentlyResizing = m_resizeContext.isResizing && m_resizeContext.resizeNode == node;
        
        // Check if mouse is over the resize handle (only matters if not already resizing)
        bool isHovered = mousePos.x >= rectMin.x && mousePos.x <= rectMax.x &&
                         mousePos.y >= rectMin.y && mousePos.y <= rectMax.y;
        
        // Start resize if hovering and mouse pressed
        bool shouldStartResize = isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_resizeContext.isResizing;
        
        // Start new resize operation
        if (shouldStartResize) {
            m_resizeContext.isResizing = true;
            m_resizeContext.resizeNode = node;
            m_resizeContext.startMousePos = mousePos;
            m_resizeContext.startSplitRatio = data.splitRatio;
            std::cout << "Started resize for node " << node->GetId() << " at ratio " << data.splitRatio << std::endl;
        }
        
        // Stop resize operation if mouse is released
        if (m_resizeContext.isResizing && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            std::cout << "Stopped resize - mouse released" << std::endl;
            m_resizeContext.Reset();
        }
        
        // Determine visual state
        bool isActivelyResizing = isCurrentlyResizing && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        
        // Debug output
        if (isHovered || isActivelyResizing) {
            std::cout << "Resize handle " << node->GetId() 
                      << " - Mouse: (" << mousePos.x << "," << mousePos.y << ")"
                      << " Handle: (" << rectMin.x << "," << rectMin.y << " to " << rectMax.x << "," << rectMax.y << ")"
                      << " Hovered: " << isHovered 
                      << " ActivelyResizing: " << isActivelyResizing << std::endl;
        }
        
        // Set cursor when hovering or actively resizing
        if (isHovered || isActivelyResizing) {
            ImGui::SetMouseCursor(cursor);
        }
        
        // Use ImGui's foreground draw list to draw the handle (invisible but functional)
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        if (drawList) {
            // Draw subtle resize handle - only visible when actively resizing
            if (isActivelyResizing) {
                ImU32 handleColor = IM_COL32(100, 150, 255, 100);  // Subtle blue when resizing
                drawList->AddRectFilled(rectMin, rectMax, handleColor);
            }
            // No visible handle when just hovering - keep it invisible but functional
        }
        
        // Handle active resizing
        if (isActivelyResizing) {
            std::cout << "RESIZE DRAGGING: Mouse at (" << mousePos.x << "," << mousePos.y << ")" << std::endl;
            
            if (data.isHorizontalSplit) {
                float newRatio = (mousePos.x - data.position.x) / data.size.x;
                newRatio = std::clamp(newRatio, 0.1f, 0.9f);
                std::cout << "Horizontal resize: old ratio = " << data.splitRatio << " new ratio = " << newRatio << std::endl;
                node->GetData().splitRatio = newRatio;
            } else {
                float newRatio = (mousePos.y - data.position.y) / data.size.y;
                newRatio = std::clamp(newRatio, 0.1f, 0.9f);
                std::cout << "Vertical resize: old ratio = " << data.splitRatio << " new ratio = " << newRatio << std::endl;
                node->GetData().splitRatio = newRatio;
            }
            
            // Recalculate layout for the entire tree
            if (m_rootNode) {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                if (viewport) {
                    CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
                }
            }
            
            resizeHandled = true;
        }
    }
    ImGui::End();
    
    return resizeHandled;
}

void DockingSystem::RenderFloatingNodes() {
    for (auto& floatingNode : m_floatingNodes) {
        if (floatingNode && !floatingNode->IsEmpty()) {
            RenderNode(floatingNode);
        }
    }
}

void DockingSystem::CleanupEmptyNodes() {
    if (m_rootNode) {
        m_rootNode->Cleanup();
    }
    
    // Remove empty floating nodes
    m_floatingNodes.erase(
        std::remove_if(m_floatingNodes.begin(), m_floatingNodes.end(),
            [](const std::shared_ptr<DockNode>& node) {
                return !node || node->IsEmpty();
            }),
        m_floatingNodes.end());
}

void DockingSystem::AddPanel(std::shared_ptr<Panel> panel, const std::string& defaultArea) {
    if (!panel) return;
    
    // Store panel reference in both registries
    m_allPanels[panel->GetName()] = panel;
    m_availablePanels[panel->GetName()] = panel;
    
    // Set up visibility change callback to handle when user closes panel via X button
    panel->SetVisibilityChangedCallback([this](const std::string& panelName, bool visible) {
        if (!visible) {
            // Panel was closed by user, remove from active panels but keep in available
            this->RemovePanel(panelName);
        }
    });
    
    std::shared_ptr<DockNode> targetNode = nullptr;
    
    // Find the appropriate dock node based on the default layout structure
    if (!defaultArea.empty() && m_rootNode) {
        if (defaultArea == "left") {
            // Navigate to left panel: Root -> Top -> Left
            auto topNode = m_rootNode->GetChildren().size() > 0 ? m_rootNode->GetChildren()[0] : nullptr;
            if (topNode && topNode->GetChildren().size() > 0) {
                targetNode = topNode->GetChildren()[0]; // Left node
            }
        }
        else if (defaultArea == "center" || defaultArea == "game") {
            // Navigate to game panel: Root -> Top -> GameInspector -> Game
            auto topNode = m_rootNode->GetChildren().size() > 0 ? m_rootNode->GetChildren()[0] : nullptr;
            if (topNode && topNode->GetChildren().size() > 1) {
                auto gameInspectorNode = topNode->GetChildren()[1];
                if (gameInspectorNode && gameInspectorNode->GetChildren().size() > 0) {
                    targetNode = gameInspectorNode->GetChildren()[0]; // Game node
                }
            }
        }
        else if (defaultArea == "right" || defaultArea == "inspector") {
            // Navigate to inspector panel: Root -> Top -> GameInspector -> Inspector
            auto topNode = m_rootNode->GetChildren().size() > 0 ? m_rootNode->GetChildren()[0] : nullptr;
            if (topNode && topNode->GetChildren().size() > 1) {
                auto gameInspectorNode = topNode->GetChildren()[1];
                if (gameInspectorNode && gameInspectorNode->GetChildren().size() > 1) {
                    targetNode = gameInspectorNode->GetChildren()[1]; // Inspector node
                }
            }
        }
        else if (defaultArea == "bottom") {
            // Navigate to bottom panel: Root -> Bottom
            if (m_rootNode->GetChildren().size() > 1) {
                targetNode = m_rootNode->GetChildren()[1]; // Bottom node
            }
        }
    }
    
    // Fallback: find any available leaf node
    if (!targetNode && m_rootNode) {
        std::function<std::shared_ptr<DockNode>(std::shared_ptr<DockNode>)> findLeaf;
        findLeaf = [&](std::shared_ptr<DockNode> node) -> std::shared_ptr<DockNode> {
            if (!node) return nullptr;
            if (node->IsLeaf()) return node;
            
            for (auto& child : node->GetChildren()) {
                auto result = findLeaf(child);
                if (result) return result;
            }
            return nullptr;
        };
        
        targetNode = findLeaf(m_rootNode);
    }
    
    if (targetNode) {
        targetNode->AddPanel(panel);
        std::cout << "Added panel '" << panel->GetName() << "' to " << defaultArea << " area" << std::endl;
    } else {
        // Create floating window as fallback
        CreateFloatingWindow(panel);
        std::cout << "Created floating window for panel '" << panel->GetName() << "'" << std::endl;
    }
}

void DockingSystem::RemovePanel(const std::string& panelName) {
    auto it = m_allPanels.find(panelName);
    if (it != m_allPanels.end()) {
        RemovePanel(it->second);
        // Remove from active panels but keep in available panels registry
        m_allPanels.erase(it);
        // Note: we DON'T remove from m_availablePanels to keep the panel available for reopening
    }
}

void DockingSystem::RemovePanel(std::shared_ptr<Panel> panel) {
    if (!panel) return;
    
    std::cout << "Removing panel: " << panel->GetName() << std::endl;
    
    // Find and remove from dock tree
    if (m_rootNode) {
        auto node = m_rootNode->FindNodeWithPanel(panel);
        if (node) {
            node->RemovePanel(panel);
            
            // Check if this node is now empty and needs cleanup
            if (node->IsEmpty()) {
                std::cout << "Node is empty after removing panel, cleaning up layout" << std::endl;
                CleanupEmptyNodes();
                
                // Recalculate layout after cleanup
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                if (viewport && m_rootNode) {
                    CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
                }
            }
        }
    }
    
    // Remove from floating windows
    for (auto& floatingNode : m_floatingNodes) {
        if (floatingNode && floatingNode->HasPanel(panel->GetName())) {
            floatingNode->RemovePanel(panel);
            break;
        }
    }
    
    // Remove from all panels map
    m_allPanels.erase(panel->GetName());
    
    std::cout << "Panel " << panel->GetName() << " removed successfully" << std::endl;
}

std::shared_ptr<Panel> DockingSystem::GetPanel(const std::string& panelName) {
    auto it = m_allPanels.find(panelName);
    return (it != m_allPanels.end()) ? it->second : nullptr;
}

void DockingSystem::FocusPanel(const std::string& panelName) {
    auto panel = GetPanel(panelName);
    if (!panel) return;
    
    // Find the node containing this panel
    if (m_rootNode) {
        auto node = m_rootNode->FindNodeWithPanel(panel);
        if (node) {
            node->SetActiveTab(panelName);
        }
    }
    
    // Check floating windows
    for (auto& floatingNode : m_floatingNodes) {
        if (floatingNode && floatingNode->HasPanel(panelName)) {
            floatingNode->SetActiveTab(panelName);
            break;
        }
    }
}

void DockingSystem::ShowPanel(const std::string& panelName, bool show) {
    // Check both active panels and available panels registry
    auto panel = GetPanel(panelName);
    if (!panel) {
        // Check if it's in the available panels but not active
        auto it = m_availablePanels.find(panelName);
        if (it != m_availablePanels.end()) {
            panel = it->second;
        }
    }
    
    if (panel) {
        panel->SetVisible(show);
        if (show) {
            // If panel is not in the active panels, add it back to docking
            if (m_allPanels.find(panelName) == m_allPanels.end()) {
                // Find a good default location - try bottom first, then floating
                std::shared_ptr<DockNode> targetNode = nullptr;
                if (m_rootNode && m_rootNode->GetChildren().size() > 1) {
                    targetNode = m_rootNode->GetChildren()[1]; // Bottom node
                }
                
                if (targetNode && targetNode->IsLeaf()) {
                    targetNode->AddPanel(panel);
                } else {
                    // Create floating window as fallback
                    CreateFloatingWindow(panel);
                }
                
                // Add back to active panels
                m_allPanels[panelName] = panel;
            }
            FocusPanel(panelName);
        } else {
            // Remove from active panels but keep in available
            RemovePanel(panelName);
        }
    }
}

void DockingSystem::TogglePanel(const std::string& panelName) {
    auto panel = GetPanel(panelName);
    if (panel) {
        ShowPanel(panelName, !panel->IsVisible());
    }
}

void DockingSystem::CreateFloatingWindow(std::shared_ptr<Panel> panel) {
    if (!panel) return;
    
    auto floatingNode = std::make_shared<DockNode>(DockNodeType::Floating);
    floatingNode->AddPanel(panel);
    
    // Set default floating window size and position
    floatingNode->GetData().position = {100, 100};
    floatingNode->GetData().size = {400, 300};
    
    m_floatingNodes.push_back(floatingNode);
}

void DockingSystem::ResetToDefaultLayout() {
    LoadUnityLayout(); // Default is Unity-style layout
}

void DockingSystem::LoadUnityLayout() {
    // Clear existing layout but preserve panel registry
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
    m_floatingNodes.clear();
    m_allPanels.clear(); // Clear active panels to rebuild
    
    // Set current base layout
    m_currentBaseLayout = "Unity";
    
    // Recreate Unity-style layout
    CreateDefaultLayout();
    
    // Re-add ALL available panels (both active and inactive) to specific Unity layout positions
    // Navigate the tree structure we created in CreateDefaultLayout
    if (m_rootNode && m_rootNode->GetChildren().size() >= 2) {
        auto topNode = m_rootNode->GetChildren()[0]; // Top section
        auto bottomNode = m_rootNode->GetChildren()[1]; // Bottom section
        
        if (topNode && topNode->GetChildren().size() >= 2) {
            auto leftNode = topNode->GetChildren()[0]; // Hierarchy
            auto gameInspectorNode = topNode->GetChildren()[1]; // Game + Inspector
            
            if (gameInspectorNode && gameInspectorNode->GetChildren().size() >= 2) {
                auto gameNode = gameInspectorNode->GetChildren()[0]; // Game
                auto inspectorNode = gameInspectorNode->GetChildren()[1]; // Inspector
                
                // Now assign ALL AVAILABLE panels to specific nodes and reopen them
                for (auto& [name, panel] : m_availablePanels) {
                    if (panel) {
                        // Reopen the panel
                        panel->SetVisible(true);
                        
                        // Add to active panels registry
                        m_allPanels[name] = panel;
                        
                        // Position according to Unity layout
                        if (name == "Hierarchy") {
                            leftNode->AddPanel(panel);
                        } else if (name == "Game") {
                            gameNode->AddPanel(panel);
                        } else if (name == "Inspector" || name == "MaterialEditor") {
                            inspectorNode->AddPanel(panel);
                        } else if (name == "Asset Browser" || name == "Materials" || name == "Console") {
                            bottomNode->AddPanel(panel);
                        } else {
                            // Default to inspector area
                            inspectorNode->AddPanel(panel);
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Switched to Unity-style layout and reopened all panels" << std::endl;
}

void DockingSystem::LoadCodeEditorLayout() {
    // Clear existing layout but preserve panel registry
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
    m_floatingNodes.clear();
    m_allPanels.clear(); // Clear active panels to rebuild
    
    // Set current base layout
    m_currentBaseLayout = "CodeEditor";
    
    // Get viewport size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) return;
    
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    
    // Set root node size
    m_rootNode->GetData().position = workPos;
    m_rootNode->GetData().size = workSize;
    
    // Code Editor layout: Game (70%) | Inspector (30%) with Hierarchy and Console at bottom
    m_rootNode->SetType(DockNodeType::Split);
    m_rootNode->GetData().isHorizontalSplit = false; // Vertical split
    m_rootNode->GetData().splitRatio = 0.75f; // 75% top, 25% bottom
    
    // Top section: Game | Inspector
    auto topNode = std::make_shared<DockNode>(DockNodeType::Split);
    topNode->GetData().isHorizontalSplit = true;
    topNode->GetData().splitRatio = 0.7f; // 70% game, 30% inspector
    m_rootNode->AddChild(topNode);
    
    auto gameNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    auto inspectorNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    topNode->AddChild(gameNode);
    topNode->AddChild(inspectorNode);
    
    // Bottom section: Hierarchy | Console
    auto bottomNode = std::make_shared<DockNode>(DockNodeType::Split);
    bottomNode->GetData().isHorizontalSplit = true;
    bottomNode->GetData().splitRatio = 0.3f; // 30% hierarchy, 70% console
    m_rootNode->AddChild(bottomNode);
    
    auto hierarchyNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    auto consoleNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    bottomNode->AddChild(hierarchyNode);
    bottomNode->AddChild(consoleNode);
    
    // Calculate initial layout
    CalculateNodeLayout(m_rootNode, workPos, workSize);
    
    // Re-add ALL available panels to specific nodes and reopen them
    for (auto& [name, panel] : m_availablePanels) {
        if (panel) {
            // Reopen the panel
            panel->SetVisible(true);
            
            // Add to active panels registry
            m_allPanels[name] = panel;
            
            // Position according to Code Editor layout
            if (name == "Game") {
                gameNode->AddPanel(panel);
            } else if (name == "Inspector" || name == "MaterialEditor") {
                inspectorNode->AddPanel(panel);
            } else if (name == "Hierarchy") {
                hierarchyNode->AddPanel(panel);
            } else if (name == "Console" || name == "Asset Browser" || name == "Materials") {
                consoleNode->AddPanel(panel);
            } else {
                // Default to console area for unknown panels
                consoleNode->AddPanel(panel);
            }
        }
    }
    
    std::cout << "Switched to Code Editor layout and reopened all panels" << std::endl;
}

void DockingSystem::LoadInspectorFocusLayout() {
    // Clear existing layout but preserve panel registry
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
    m_floatingNodes.clear();
    m_allPanels.clear(); // Clear active panels to rebuild
    
    // Set current base layout
    m_currentBaseLayout = "InspectorFocus";
    
    // Get viewport size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) return;
    
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    
    // Set root node size
    m_rootNode->GetData().position = workPos;
    m_rootNode->GetData().size = workSize;
    
    // Inspector Focus: Left Hierarchy | Center Game | Right Large Inspector
    m_rootNode->SetType(DockNodeType::Split);
    m_rootNode->GetData().isHorizontalSplit = true;
    m_rootNode->GetData().splitRatio = 0.15f; // 15% left hierarchy
    
    // Left: Hierarchy
    auto hierarchyNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    m_rootNode->AddChild(hierarchyNode);
    
    // Right: Game | Inspector
    auto rightNode = std::make_shared<DockNode>(DockNodeType::Split);
    rightNode->GetData().isHorizontalSplit = true;
    rightNode->GetData().splitRatio = 0.5f; // 50% game, 50% inspector
    m_rootNode->AddChild(rightNode);
    
    auto gameNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    auto inspectorNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    rightNode->AddChild(gameNode);
    rightNode->AddChild(inspectorNode);
    
    // Calculate initial layout
    CalculateNodeLayout(m_rootNode, workPos, workSize);
    
    // Re-add ALL available panels to specific nodes and reopen them
    for (auto& [name, panel] : m_availablePanels) {
        if (panel) {
            // Reopen the panel
            panel->SetVisible(true);
            
            // Add to active panels registry
            m_allPanels[name] = panel;
            
            // Position according to Inspector Focus layout
            if (name == "Hierarchy") {
                hierarchyNode->AddPanel(panel);
            } else if (name == "Game") {
                gameNode->AddPanel(panel);
            } else if (name == "Inspector" || name == "MaterialEditor" || name == "Asset Browser") {
                inspectorNode->AddPanel(panel);
            } else if (name == "Materials" || name == "Console") {
                // Add to inspector as tabs
                inspectorNode->AddPanel(panel);
            } else {
                inspectorNode->AddPanel(panel);
            }
        }
    }
    
    std::cout << "Switched to Inspector Focus layout and reopened all panels" << std::endl;
}

void DockingSystem::LoadGameFocusLayout() {
    // Clear existing layout but preserve panel registry
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
    m_floatingNodes.clear();
    m_allPanels.clear(); // Clear active panels to rebuild
    
    // Set current base layout
    m_currentBaseLayout = "GameFocus";
    
    // Get viewport size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) return;
    
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    
    // Set root node size
    m_rootNode->GetData().position = workPos;
    m_rootNode->GetData().size = workSize;
    
    // Game Focus: Large Game with minimal bottom controls
    m_rootNode->SetType(DockNodeType::Split);
    m_rootNode->GetData().isHorizontalSplit = false; // Vertical split
    m_rootNode->GetData().splitRatio = 0.85f; // 85% game, 15% bottom
    
    // Top: Large Game
    auto gameNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    m_rootNode->AddChild(gameNode);
    
    // Bottom: Controls (Hierarchy | Inspector | Materials)
    auto bottomNode = std::make_shared<DockNode>(DockNodeType::Split);
    bottomNode->GetData().isHorizontalSplit = true;
    bottomNode->GetData().splitRatio = 0.33f; // Three equal sections
    m_rootNode->AddChild(bottomNode);
    
    auto leftBottomNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    bottomNode->AddChild(leftBottomNode);
    
    auto rightBottomNode = std::make_shared<DockNode>(DockNodeType::Split);
    rightBottomNode->GetData().isHorizontalSplit = true;
    rightBottomNode->GetData().splitRatio = 0.5f;
    bottomNode->AddChild(rightBottomNode);
    
    auto middleBottomNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    auto rightmostBottomNode = std::make_shared<DockNode>(DockNodeType::Leaf);
    rightBottomNode->AddChild(middleBottomNode);
    rightBottomNode->AddChild(rightmostBottomNode);
    
    // Calculate initial layout
    CalculateNodeLayout(m_rootNode, workPos, workSize);
    
    // Re-add ALL available panels to specific nodes and reopen them
    for (auto& [name, panel] : m_availablePanels) {
        if (panel) {
            // Reopen the panel
            panel->SetVisible(true);
            
            // Add to active panels registry
            m_allPanels[name] = panel;
            
            // Position according to Game Focus layout
            if (name == "Game") {
                gameNode->AddPanel(panel);
            } else if (name == "Hierarchy") {
                leftBottomNode->AddPanel(panel);
            } else if (name == "Inspector" || name == "MaterialEditor") {
                middleBottomNode->AddPanel(panel);
            } else if (name == "Materials" || name == "Asset Browser" || name == "Console") {
                rightmostBottomNode->AddPanel(panel);
            } else {
                rightmostBottomNode->AddPanel(panel);
            }
        }
    }
    
    std::cout << "Switched to Game Focus layout and reopened all panels" << std::endl;
}

void DockingSystem::SaveLayout(const std::string& filename) {
    if (!m_rootNode) return;
    
    // First save the dock tree structure
    LayoutSerializer::SaveLayoutToFile(m_rootNode, filename);
    
    // Now save panel assignments in a separate file
    std::string panelFile = filename.substr(0, filename.find_last_of('.')) + "_panels.json";
    SavePanelAssignments(panelFile);
}

void DockingSystem::LoadLayout(const std::string& filename) {
    auto loadedRoot = LayoutSerializer::LoadLayoutFromFile(filename);
    if (!loadedRoot) return;
    
    // Store current panels before clearing
    auto currentPanels = m_allPanels;
    
    // Replace root node with loaded structure
    m_rootNode = loadedRoot;
    m_floatingNodes.clear();
    
    // Calculate layout for new structure
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport) {
        CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
    }
    
    // Load panel assignments
    std::string panelFile = filename.substr(0, filename.find_last_of('.')) + "_panels.json";
    LoadPanelAssignments(panelFile, currentPanels);
}

void DockingSystem::SavePanelAssignments(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"panels\": [\n";
    
    bool first = true;
    std::function<void(std::shared_ptr<DockNode>, const std::string&)> savePanels;
    savePanels = [&](std::shared_ptr<DockNode> node, const std::string& path) {
        if (!node) return;
        
        if (node->IsLeaf() && !node->IsEmpty()) {
            for (const auto& panel : node->GetPanels()) {
                if (!first) file << ",\n";
                first = false;
                
                file << "    {\n";
                file << "      \"name\": \"" << panel->GetName() << "\",\n";
                file << "      \"path\": \"" << path << "\"\n";
                file << "    }";
            }
        }
        
        // Traverse children with path tracking
        for (size_t i = 0; i < node->GetChildren().size(); ++i) {
            std::string childPath = path.empty() ? std::to_string(i) : path + "." + std::to_string(i);
            savePanels(node->GetChildren()[i], childPath);
        }
    };
    
    savePanels(m_rootNode, "");
    
    file << "\n  ]\n";
    file << "}\n";
    file.close();
}

void DockingSystem::LoadPanelAssignments(const std::string& filename, 
                                        const std::unordered_map<std::string, std::shared_ptr<Panel>>& panels) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // No panel assignments file - use default placement
        for (auto& [name, panel] : panels) {
            if (panel) {
                AddPanel(panel, "");
            }
        }
        return;
    }
    
    // Simple JSON parsing (this is a simplified version)
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Find all panel entries
    size_t pos = 0;
    while ((pos = content.find("\"name\":", pos)) != std::string::npos) {
        pos += 8; // Skip "name": "
        size_t nameStart = content.find("\"", pos) + 1;
        size_t nameEnd = content.find("\"", nameStart);
        std::string panelName = content.substr(nameStart, nameEnd - nameStart);
        
        // Find corresponding path
        pos = content.find("\"path\":", nameEnd);
        if (pos != std::string::npos) {
            pos += 8; // Skip "path": "
            size_t pathStart = content.find("\"", pos) + 1;
            size_t pathEnd = content.find("\"", pathStart);
            std::string nodePath = content.substr(pathStart, pathEnd - pathStart);
            
            // Find the panel and node by path
            auto panelIt = panels.find(panelName);
            if (panelIt != panels.end() && panelIt->second) {
                auto node = FindNodeByPath(m_rootNode, nodePath);
                if (node && node->IsLeaf()) {
                    node->AddPanel(panelIt->second);
                } else {
                    // Fallback to default placement
                    AddPanel(panelIt->second, "");
                }
            }
        }
    }
    
    // Add any panels that weren't in the saved layout
    for (auto& [name, panel] : panels) {
        if (panel) {
            std::function<bool(std::shared_ptr<DockNode>)> hasPanel;
            hasPanel = [&](std::shared_ptr<DockNode> node) -> bool {
                if (!node) return false;
                if (node->HasPanel(name)) return true;
                for (const auto& child : node->GetChildren()) {
                    if (hasPanel(child)) return true;
                }
                return false;
            };
            
            if (!hasPanel(m_rootNode)) {
                AddPanel(panel, "");
            }
        }
    }
}

std::shared_ptr<DockNode> DockingSystem::FindNodeById(std::shared_ptr<DockNode> node, const std::string& id) {
    if (!node) return nullptr;
    if (node->GetId() == id) return node;
    
    for (const auto& child : node->GetChildren()) {
        auto found = FindNodeById(child, id);
        if (found) return found;
    }
    
    return nullptr;
}

std::shared_ptr<DockNode> DockingSystem::FindNodeByPath(std::shared_ptr<DockNode> node, const std::string& path) {
    if (!node) return nullptr;
    if (path.empty()) return node;
    
    // Parse path (e.g., "0.1.2" means child[0]->child[1]->child[2])
    size_t dotPos = path.find('.');
    
    std::string indexStr = (dotPos == std::string::npos) ? path : path.substr(0, dotPos);
    int childIndex = std::stoi(indexStr);
    
    if (childIndex >= 0 && childIndex < static_cast<int>(node->GetChildren().size())) {
        auto child = node->GetChildren()[childIndex];
        
        if (dotPos == std::string::npos) {
            // This was the last index in the path
            return child;
        } else {
            // Continue traversing
            std::string remainingPath = path.substr(dotPos + 1);
            return FindNodeByPath(child, remainingPath);
        }
    }
    
    return nullptr;
}

void DockingSystem::DockFloatingWindow(std::shared_ptr<DockNode> floatingNode, 
                                      std::shared_ptr<DockNode> targetNode, 
                                      DockDirection direction) {
    if (!floatingNode || !targetNode || !floatingNode->IsFloating()) return;
    
    // Get panels from floating node
    auto panels = floatingNode->GetPanels();
    
    // Remove floating node
    auto it = std::find(m_floatingNodes.begin(), m_floatingNodes.end(), floatingNode);
    if (it != m_floatingNodes.end()) {
        m_floatingNodes.erase(it);
    }
    
    // Dock each panel
    for (auto& panel : panels) {
        if (panel) {
            DockPanel(panel, targetNode, direction);
        }
    }
}

void DockingSystem::SaveCustomLayout(const std::string& layoutName) {
    if (layoutName.empty() || !m_rootNode) return;
    
    // Capture current layout as a function that recreates it
    PersistentLayoutInfo layoutInfo = CaptureCurrentLayout(layoutName);
    m_persistentLayouts[layoutName] = layoutInfo;
    
    // Save to disk for persistence across sessions
    std::string layoutsDir = "layouts/";
    if (!std::filesystem::exists(layoutsDir)) {
        std::filesystem::create_directories(layoutsDir);
    }
    
    std::string filename = layoutsDir + layoutName + ".json";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << layoutInfo.Serialize();
        file.close();
        std::cout << "Saved custom layout to disk: " << filename << std::endl;
    }
    
    std::cout << "Saved custom layout: " << layoutName << std::endl;
}

bool DockingSystem::LoadCustomLayout(const std::string& layoutName) {
    // Check persistent layouts first
    auto persistentIt = m_persistentLayouts.find(layoutName);
    if (persistentIt != m_persistentLayouts.end()) {
        return RestoreLayout(persistentIt->second);
    }
    
    // Try loading from disk
    std::string filename = "layouts/" + layoutName + ".json";
    if (std::filesystem::exists(filename)) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            
            PersistentLayoutInfo layoutInfo;
            if (layoutInfo.Deserialize(content)) {
                m_persistentLayouts[layoutName] = layoutInfo; // Cache in memory
                return RestoreLayout(layoutInfo);
            }
        }
    }
    
    // Fallback to legacy in-memory layouts
    auto legacyIt = m_savedLayouts.find(layoutName);
    if (legacyIt != m_savedLayouts.end()) {
        legacyIt->second(this);
        std::cout << "Loaded legacy layout: " << layoutName << std::endl;
        return true;
    }
    
    std::cout << "Layout not found: " << layoutName << std::endl;
    return false;
}

std::shared_ptr<DockNode> DockingSystem::CloneNodeTree(std::shared_ptr<DockNode> node) {
    if (!node) return nullptr;
    
    auto clone = std::make_shared<DockNode>(node->GetType());
    clone->GetData() = node->GetData();
    
    // Clone panels for leaf nodes
    if (node->IsLeaf()) {
        for (const auto& panel : node->GetPanels()) {
            clone->AddPanel(panel);
        }
    }
    
    // Recursively clone children
    for (const auto& child : node->GetChildren()) {
        clone->AddChild(CloneNodeTree(child));
    }
    
    return clone;
}

void DockingSystem::RestorePanelsToClonedTree(std::shared_ptr<DockNode> targetTree, 
                                             std::shared_ptr<DockNode> sourceTree,
                                             const std::unordered_map<std::string, std::shared_ptr<Panel>>& panels) {
    if (!targetTree || !sourceTree) return;
    
    // If source node had panels, add them to target
    if (sourceTree->IsLeaf() && !sourceTree->IsEmpty()) {
        for (const auto& panel : sourceTree->GetPanels()) {
            auto it = panels.find(panel->GetName());
            if (it != panels.end()) {
                targetTree->AddPanel(it->second);
            }
        }
    }
    
    // Recursively restore children
    auto& targetChildren = targetTree->GetChildren();
    auto& sourceChildren = sourceTree->GetChildren();
    
    for (size_t i = 0; i < targetChildren.size() && i < sourceChildren.size(); ++i) {
        RestorePanelsToClonedTree(targetChildren[i], sourceChildren[i], panels);
    }
}

std::vector<std::string> DockingSystem::GetSavedLayouts() const {
    std::vector<std::string> layouts;
    std::string layoutDir = "layouts/";
    
    // Add persistent layouts first
    for (const auto& [name, layoutInfo] : m_persistentLayouts) {
        layouts.push_back(name);
    }
    
    // Add legacy in-memory layouts
    for (const auto& [name, func] : m_savedLayouts) {
        if (std::find(layouts.begin(), layouts.end(), name) == layouts.end()) {
            layouts.push_back(name);
        }
    }
    
    // Check if directory exists
    if (!std::filesystem::exists(layoutDir)) {
        return layouts;
    }
    
    // Iterate through files in the layouts directory
    for (const auto& entry : std::filesystem::directory_iterator(layoutDir)) {
        if (entry.is_regular_file() && 
           (entry.path().extension() == ".json" || entry.path().extension() == ".layout")) {
            // Get filename without extension
            std::string filename = entry.path().stem().string();
            
            // Avoid duplicates from in-memory layouts
            if (std::find(layouts.begin(), layouts.end(), filename) == layouts.end()) {
                layouts.push_back(filename);
            }
        }
    }
    
    return layouts;
}

void DockingSystem::DeleteCustomLayout(const std::string& layoutName) {
    if (layoutName.empty()) return;
    
    bool foundLayout = false;
    
    // Remove from persistent layouts (new system)
    auto persistentIt = m_persistentLayouts.find(layoutName);
    if (persistentIt != m_persistentLayouts.end()) {
        m_persistentLayouts.erase(persistentIt);
        foundLayout = true;
        std::cout << "Deleted layout from memory: " << layoutName << std::endl;
    }
    
    // Remove from legacy in-memory layouts
    auto legacyIt = m_savedLayouts.find(layoutName);
    if (legacyIt != m_savedLayouts.end()) {
        m_savedLayouts.erase(legacyIt);
        foundLayout = true;
        std::cout << "Deleted legacy layout from memory: " << layoutName << std::endl;
    }
    
    // Remove from disk
    std::string layoutDir = "layouts/";
    std::string fullPath = layoutDir + layoutName + ".json";
    
    if (std::filesystem::exists(fullPath)) {
        std::filesystem::remove(fullPath);
        foundLayout = true;
        std::cout << "Deleted layout from disk: " << layoutName << std::endl;
    }
    
    if (!foundLayout) {
        std::cout << "Layout not found: " << layoutName << std::endl;
    } else {
        std::cout << "Successfully deleted layout: " << layoutName << std::endl;
    }
}

void DockingSystem::RenderSaveLayoutDialog() {
    if (!m_showSaveDialog) {
        return;
    }
    
    ImGui::OpenPopup("Save Layout");
    
    // Center the dialog
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 120));
    
    if (ImGui::BeginPopupModal("Save Layout", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter a name for this layout:");
        
        if (ImGui::InputText("##LayoutName", m_saveDialogBuffer, sizeof(m_saveDialogBuffer), ImGuiInputTextFlags_AutoSelectAll)) {
            // Input handled
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            if (strlen(m_saveDialogBuffer) > 0) {
                SaveCustomLayout(m_saveDialogBuffer);
                HideSaveDialog();
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            HideSaveDialog();
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

PersistentLayoutInfo DockingSystem::CaptureCurrentLayout(const std::string& layoutName) const {
    PersistentLayoutInfo layoutInfo;
    layoutInfo.name = layoutName;
    
    // Capture the actual layout structure, not just preset base
    if (m_rootNode) {
        layoutInfo.rootNode = CaptureNodeData(m_rootNode);
    }
    
    // Capture floating nodes
    for (const auto& floatingNode : m_floatingNodes) {
        layoutInfo.floatingNodes.push_back(CaptureNodeData(floatingNode));
    }
    
    // Capture panel visibility state for all available panels
    for (const auto& [panelName, panel] : m_availablePanels) {
        layoutInfo.panelVisibility[panelName] = panel->IsVisible();
    }
    
    std::cout << "Captured layout '" << layoutName << "' with " << layoutInfo.panelVisibility.size() << " panels and actual structure" << std::endl;
    
    return layoutInfo;
}

bool DockingSystem::RestoreLayout(const PersistentLayoutInfo& layoutInfo) {
    // Clear current layout
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
    m_floatingNodes.clear();
    m_allPanels.clear();
    
    // Restore the actual layout structure
    if (layoutInfo.rootNode.children.empty() && layoutInfo.rootNode.panelNames.empty()) {
        // Old format without structure - fall back to preset layout
        if (!layoutInfo.baseLayout.empty()) {
            if (layoutInfo.baseLayout == "Unity") {
                LoadUnityLayout();
            } else if (layoutInfo.baseLayout == "CodeEditor") {
                LoadCodeEditorLayout();
            } else if (layoutInfo.baseLayout == "InspectorFocus") {
                LoadInspectorFocusLayout();
            } else if (layoutInfo.baseLayout == "GameFocus") {
                LoadGameFocusLayout();
            } else {
                LoadUnityLayout();
            }
        } else {
            LoadUnityLayout();
        }
    } else {
        // New format with actual structure - restore it
        m_rootNode = CreateNodeFromLayoutData(layoutInfo.rootNode);
        
        // Restore floating nodes
        for (const auto& floatingData : layoutInfo.floatingNodes) {
            auto floatingNode = CreateNodeFromLayoutData(floatingData);
            if (floatingNode) {
                m_floatingNodes.push_back(floatingNode);
            }
        }
        
        // Rebuild m_allPanels registry by traversing the restored node tree
        std::function<void(std::shared_ptr<DockNode>)> collectPanels = [&](std::shared_ptr<DockNode> node) {
            if (!node) return;
            
            if (node->IsLeaf()) {
                for (const auto& panel : node->GetPanels()) {
                    if (panel) {
                        m_allPanels[panel->GetName()] = panel;
                        panel->SetVisible(true); // Panel is visible if it's in the layout
                    }
                }
            }
            
            for (const auto& child : node->GetChildren()) {
                collectPanels(child);
            }
        };
        
        // Collect panels from root node
        collectPanels(m_rootNode);
        
        // Collect panels from floating nodes
        for (const auto& floatingNode : m_floatingNodes) {
            collectPanels(floatingNode);
        }
    }
    
    // Set visibility for all available panels based on saved state
    for (const auto& [panelName, panel] : m_availablePanels) {
        auto it = layoutInfo.panelVisibility.find(panelName);
        if (it != layoutInfo.panelVisibility.end()) {
            panel->SetVisible(it->second);
        }
    }
    
    // Calculate layout for current viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport) {
        CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
    }
    
    std::cout << "Restored layout: " << layoutInfo.name << " with " << m_allPanels.size() << " active panels and actual structure" << std::endl;
    return true;
}

std::shared_ptr<DockNode> DockingSystem::CreateNodeFromLayoutData(const LayoutNodeData& data) {
    // Convert LayoutNodeType to DockNodeType
    DockNodeType nodeType;
    switch(data.type) {
        case LayoutNodeType::Root: nodeType = DockNodeType::Root; break;
        case LayoutNodeType::Split: nodeType = DockNodeType::Split; break;
        case LayoutNodeType::Leaf: nodeType = DockNodeType::Leaf; break;
        case LayoutNodeType::Floating: nodeType = DockNodeType::Floating; break;
        default: nodeType = DockNodeType::Leaf; break;
    }
    
    auto node = std::make_shared<DockNode>(nodeType);
    std::cout << "Creating node of type: " << static_cast<int>(data.type) << " with " << data.panelNames.size() << " panels" << std::endl;
    
    // Set node data
    node->GetData().position = data.position;
    node->GetData().size = data.size;
    node->GetData().splitRatio = data.splitRatio;
    node->GetData().isHorizontalSplit = data.isHorizontalSplit;
    node->GetData().activeTabIndex = data.activeTabIndex;
    node->SetId(data.nodeId);
    
    // Add panels for leaf nodes
    if (data.type == LayoutNodeType::Leaf) {
        for (const auto& panelName : data.panelNames) {
            auto panelIt = m_availablePanels.find(panelName);
            if (panelIt != m_availablePanels.end()) {
                auto panel = panelIt->second;
                node->AddPanel(panel);
                
                // Ensure visibility callback is set up (it might have been lost)
                panel->SetVisibilityChangedCallback([this](const std::string& name, bool visible) {
                    if (!visible) {
                        this->RemovePanel(name);
                    }
                });
                
                std::cout << "  Restored panel: " << panelName << std::endl;
            } else {
                std::cout << "  Panel not found in available panels: " << panelName << std::endl;
            }
        }
    }
    
    // Recursively create children
    for (const auto& childData : data.children) {
        auto childNode = CreateNodeFromLayoutData(childData);
        if (childNode) {
            node->AddChild(childNode);
        }
    }
    
    return node;
}

LayoutNodeData DockingSystem::CaptureNodeData(std::shared_ptr<DockNode> node) const {
    LayoutNodeData data;
    
    if (!node) return data;
    
    // Capture basic node info - convert DockNodeType to LayoutNodeType
    switch(node->GetType()) {
        case DockNodeType::Root: data.type = LayoutNodeType::Root; break;
        case DockNodeType::Split: data.type = LayoutNodeType::Split; break;
        case DockNodeType::Leaf: data.type = LayoutNodeType::Leaf; break;
        case DockNodeType::Floating: data.type = LayoutNodeType::Floating; break;
    }
    data.position = node->GetData().position;
    data.size = node->GetData().size;
    data.splitRatio = node->GetData().splitRatio;
    data.isHorizontalSplit = node->GetData().isHorizontalSplit;
    data.activeTabIndex = node->GetData().activeTabIndex;
    data.nodeId = node->GetId();
    
    // Capture panel names for leaf nodes
    if (node->IsLeaf()) {
        for (const auto& panel : node->GetPanels()) {
            if (panel) {
                data.panelNames.push_back(panel->GetName());
                std::cout << "  Captured panel: " << panel->GetName() << std::endl;
            }
        }
    }
    
    // Recursively capture children
    for (const auto& child : node->GetChildren()) {
        data.children.push_back(CaptureNodeData(child));
    }
    
    return data;
}

} // namespace BGE