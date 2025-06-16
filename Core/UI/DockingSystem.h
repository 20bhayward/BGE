#pragma once

#include "DockNode.h"
#include "Panel.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <imgui.h>

namespace BGE {

struct DragContext {
    bool isDragging = false;
    std::shared_ptr<Panel> draggedPanel = nullptr;
    std::shared_ptr<DockNode> sourceNode = nullptr;
    ImVec2 dragStartPos{0, 0};
    ImVec2 dragOffset{0, 0};
    
    void Reset() {
        isDragging = false;
        draggedPanel = nullptr;
        sourceNode = nullptr;
        dragStartPos = {0, 0};
        dragOffset = {0, 0};
    }
};

struct DropZone {
    ImVec2 minPos;
    ImVec2 maxPos;
    DockDirection direction;
    std::shared_ptr<DockNode> targetNode;
    bool isHighlighted = false;
    
    // Helper methods
    bool Contains(ImVec2 point) const {
        return point.x >= minPos.x && point.x <= maxPos.x &&
               point.y >= minPos.y && point.y <= maxPos.y;
    }
};

class DockingSystem {
public:
    DockingSystem();
    ~DockingSystem() = default;
    
    // Initialization
    void Initialize();
    void Shutdown();
    
    // Main rendering
    void Render();
    
    // Panel management
    void AddPanel(std::shared_ptr<Panel> panel, const std::string& defaultArea = "");
    void RemovePanel(const std::string& panelName);
    void RemovePanel(std::shared_ptr<Panel> panel);
    std::shared_ptr<Panel> GetPanel(const std::string& panelName);
    
    // Focus management
    void FocusPanel(const std::string& panelName);
    void ShowPanel(const std::string& panelName, bool show = true);
    void HidePanel(const std::string& panelName) { ShowPanel(panelName, false); }
    void TogglePanel(const std::string& panelName);
    
    // Layout management
    void ResetToDefaultLayout();
    void SaveLayout(const std::string& filename = "layout.json");
    void LoadLayout(const std::string& filename = "layout.json");
    
    // Floating windows
    void CreateFloatingWindow(std::shared_ptr<Panel> panel);
    void DockFloatingWindow(std::shared_ptr<DockNode> floatingNode, 
                           std::shared_ptr<DockNode> targetNode, 
                           DockDirection direction);
    
    // Root node access
    std::shared_ptr<DockNode> GetRootNode() const { return m_rootNode; }
    
    // Drag and drop state
    bool IsDragging() const { return m_dragContext.isDragging; }
    const DragContext& GetDragContext() const { return m_dragContext; }

private:
    std::shared_ptr<DockNode> m_rootNode;
    std::vector<std::shared_ptr<DockNode>> m_floatingNodes;
    std::unordered_map<std::string, std::shared_ptr<Panel>> m_allPanels;
    
    DragContext m_dragContext;
    std::vector<DropZone> m_dropZones;
    
    // Rendering
    void RenderNode(std::shared_ptr<DockNode> node);
    void RenderLeafNode(std::shared_ptr<DockNode> node);
    void RenderSplitNode(std::shared_ptr<DockNode> node);
    void RenderFloatingNodes();
    
    // Tab rendering
    void RenderTabBar(std::shared_ptr<DockNode> node);
    bool IsTabDragging(std::shared_ptr<DockNode> node, int tabIndex);
    
    // Drag and drop
    void UpdateDragAndDrop();
    void StartDrag(std::shared_ptr<Panel> panel, std::shared_ptr<DockNode> sourceNode);
    void UpdateDrag();
    void EndDrag();
    
    // Drop zones
    void UpdateDropZones();
    void CalculateDropZones(std::shared_ptr<DockNode> node);
    void RenderDropZones();
    DropZone* GetHoveredDropZone();
    
    // Docking operations
    void DockPanel(std::shared_ptr<Panel> panel, 
                   std::shared_ptr<DockNode> targetNode, 
                   DockDirection direction);
    
    // Layout calculation
    void CalculateNodeLayout(std::shared_ptr<DockNode> node, ImVec2 position, ImVec2 size);
    void UpdateSplitRatios();
    
    // Resize handles
    void RenderResizeHandles(std::shared_ptr<DockNode> node);
    bool HandleSplitResize(std::shared_ptr<DockNode> node);
    
    // Utility
    std::shared_ptr<DockNode> FindNodeContainingPoint(ImVec2 point);
    void CleanupEmptyNodes();
    void CreateDefaultLayout();
    
    // Constants
    static constexpr float TAB_HEIGHT = 25.0f;
    static constexpr float RESIZE_HANDLE_SIZE = 8.0f;  // Larger resize handles
    static constexpr float MIN_NODE_SIZE = 20.0f;      // Allow smaller panels
    static constexpr float DROP_ZONE_SIZE = 40.0f;
    static constexpr ImU32 DROP_ZONE_COLOR = IM_COL32(70, 130, 200, 100);
    static constexpr ImU32 DROP_ZONE_BORDER_COLOR = IM_COL32(70, 130, 200, 255);
};

} // namespace BGE