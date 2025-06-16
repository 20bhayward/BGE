#include "DockingSystem.h"
#include "LayoutSerializer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <functional>

namespace BGE {

// Constants for docking system
static constexpr float TAB_HEIGHT = 24.0f;

DockingSystem::DockingSystem() {
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
}

void DockingSystem::Initialize() {
    CreateDefaultLayout();
}

void DockingSystem::Shutdown() {
    m_allPanels.clear();
    m_floatingNodes.clear();
    m_rootNode.reset();
    m_dragContext.Reset();
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
    
    // Calculate layout for current viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport) {
        CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
    }
    
    // Update drag and drop
    UpdateDragAndDrop();
    
    // Render main docking area
    RenderNode(m_rootNode);
    
    // Render floating windows
    RenderFloatingNodes();
    
    // Render drop zones if dragging
    if (m_dragContext.isDragging) {
        RenderDropZones();
    }
    
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
            // For single panels, create a draggable header
            auto activePanel = panels[0];
            if (activePanel) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 0.3f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.4f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 0.5f));
                
                if (ImGui::Button(activePanel->GetName().c_str(), ImVec2(-1, TAB_HEIGHT))) {
                    // Button clicked
                }
                
                // Check for drag start on the button
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) && 
                    !m_dragContext.isDragging) {
                    StartDrag(activePanel, node);
                }
                
                ImGui::PopStyleColor(3);
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
    RenderResizeHandles(node);
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
            
            // Check for tab drag start after EndTabItem - more responsive with lower threshold
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) && 
                !m_dragContext.isDragging) {
                StartDrag(panel, node);
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
    if (m_dragContext.isDragging) {
        UpdateDrag();
        
        // Check for drag end
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            EndDrag();
        }
    }
    
    // Update drop zones if dragging
    if (m_dragContext.isDragging) {
        UpdateDropZones();
    }
}

void DockingSystem::StartDrag(std::shared_ptr<Panel> panel, std::shared_ptr<DockNode> sourceNode) {
    if (!panel || !sourceNode) return;
    
    m_dragContext.isDragging = true;
    m_dragContext.draggedPanel = panel;
    m_dragContext.sourceNode = sourceNode;
    m_dragContext.dragStartPos = ImGui::GetMousePos();
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

DropZone* DockingSystem::GetHoveredDropZone() {
    ImVec2 mousePos = ImGui::GetMousePos();
    
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
    // Note: children variable removed as it's not used in this function
    
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
    
    ImRect handleRect(handlePos, {handlePos.x + handleSize.x, handlePos.y + handleSize.y});
    bool isHovered = handleRect.Contains(ImGui::GetMousePos());
    bool isClicked = isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    
    // Simplified dragging detection
    static std::shared_ptr<DockNode> draggingNode = nullptr;
    bool isDragging = false;
    
    if (isClicked) {
        draggingNode = node;
    }
    
    if (draggingNode == node && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        isDragging = true;
    }
    
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        draggingNode = nullptr;
    }
    
    // Draw visible resize handle
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList) {
        ImU32 handleColor = isDragging ? IM_COL32(100, 150, 255, 200) : 
                           isHovered ? IM_COL32(150, 150, 150, 150) : 
                           IM_COL32(100, 100, 100, 100);
        drawList->AddRectFilled(handleRect.Min, handleRect.Max, handleColor);
    }
    
    if (isHovered) {
        ImGui::SetMouseCursor(cursor);
    }
    
    if (isDragging) {
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        
        if (data.isHorizontalSplit) {
            float newRatio = (ImGui::GetMousePos().x - data.position.x) / data.size.x;
            newRatio = std::clamp(newRatio, 0.02f, 0.98f); // Even more flexible - 2% to 98%
            node->GetData().splitRatio = newRatio;
            std::cout << "Horizontal split ratio: " << newRatio << std::endl;
        } else {
            float newRatio = (ImGui::GetMousePos().y - data.position.y) / data.size.y;
            newRatio = std::clamp(newRatio, 0.02f, 0.98f); // Even more flexible - 2% to 98%
            node->GetData().splitRatio = newRatio;
            std::cout << "Vertical split ratio: " << newRatio << std::endl;
        }
        
        // Recalculate layout for the entire tree
        if (m_rootNode) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            if (viewport) {
                CalculateNodeLayout(m_rootNode, viewport->WorkPos, viewport->WorkSize);
            }
        }
        
        return true;
    }
    
    return false;
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
    
    // Store panel reference
    m_allPanels[panel->GetName()] = panel;
    
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
        m_allPanels.erase(it);
    }
}

void DockingSystem::RemovePanel(std::shared_ptr<Panel> panel) {
    if (!panel) return;
    
    // Find and remove from dock tree
    if (m_rootNode) {
        auto node = m_rootNode->FindNodeWithPanel(panel);
        if (node) {
            node->RemovePanel(panel);
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
    auto panel = GetPanel(panelName);
    if (panel) {
        panel->SetVisible(show);
        if (show) {
            FocusPanel(panelName);
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
    // Clear existing layout
    m_rootNode = std::make_shared<DockNode>(DockNodeType::Root);
    m_floatingNodes.clear();
    
    // Recreate default layout
    CreateDefaultLayout();
    
    // Re-add all panels to the new layout
    for (auto& [name, panel] : m_allPanels) {
        if (panel) {
            AddPanel(panel);
        }
    }
}

void DockingSystem::SaveLayout(const std::string& filename) {
    if (m_rootNode) {
        LayoutSerializer::SaveLayoutToFile(m_rootNode, filename);
    }
}

void DockingSystem::LoadLayout(const std::string& filename) {
    auto loadedRoot = LayoutSerializer::LoadLayoutFromFile(filename);
    if (loadedRoot) {
        // Store current panels
        auto currentPanels = m_allPanels;
        
        // Replace root node
        m_rootNode = loadedRoot;
        m_floatingNodes.clear();
        
        // Re-add panels to the loaded layout
        // This is a simplified approach - in a full implementation,
        // we'd restore panels to their exact saved positions
        for (auto& [name, panel] : currentPanels) {
            if (panel) {
                AddPanel(panel, "");
            }
        }
    }
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

} // namespace BGE