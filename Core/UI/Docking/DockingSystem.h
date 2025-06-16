#pragma once

#include "DockNode.h"
#include "LayoutData.h"
#include "../Framework/Panel.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
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
    const std::unordered_map<std::string, std::shared_ptr<Panel>>& GetAllAvailablePanels() const { return m_availablePanels; }
    
    // Focus management
    void FocusPanel(const std::string& panelName);
    void ShowPanel(const std::string& panelName, bool show = true);
    void HidePanel(const std::string& panelName) { ShowPanel(panelName, false); }
    void TogglePanel(const std::string& panelName);
    
    // Layout management
    void ResetToDefaultLayout();
    void LoadUnityLayout();
    void LoadCodeEditorLayout();
    void LoadInspectorFocusLayout();
    void LoadGameFocusLayout();
    void SaveLayout(const std::string& filename = "layout.json");
    void LoadLayout(const std::string& filename = "layout.json");
    
    // Custom layout management
    void SaveCustomLayout(const std::string& layoutName);
    bool LoadCustomLayout(const std::string& layoutName);
    std::vector<std::string> GetSavedLayouts() const;
    void DeleteCustomLayout(const std::string& layoutName);
    
    // Layout dialog state
    bool IsShowingSaveDialog() const { return m_showSaveDialog; }
    void ShowSaveDialog() { m_showSaveDialog = true; }
    void HideSaveDialog() { m_showSaveDialog = false; }
    const char* GetSaveDialogBuffer() { return m_saveDialogBuffer; }
    void RenderSaveLayoutDialog();
    
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
    
    // Resize context for global resize tracking
    struct ResizeContext {
        bool isResizing = false;
        std::shared_ptr<DockNode> resizeNode = nullptr;
        ImVec2 startMousePos{0, 0};
        float startSplitRatio = 0.0f;
        
        void Reset() {
            isResizing = false;
            resizeNode = nullptr;
            startMousePos = {0, 0};
            startSplitRatio = 0.0f;
        }
    } m_resizeContext;
    
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
    void UpdateDragAndDropSafe();
    void StartDrag(std::shared_ptr<Panel> panel, std::shared_ptr<DockNode> sourceNode);
    void StartDragSafe(std::shared_ptr<Panel> panel, std::shared_ptr<DockNode> sourceNode, ImVec2 mousePos);
    void UpdateDrag();
    void EndDrag();
    void EndDragSafe();
    
    // Drop zones
    void UpdateDropZones();
    void CalculateDropZones(std::shared_ptr<DockNode> node);
    void RenderDropZones();
    void RenderDropZonesSafe();
    DropZone* GetHoveredDropZone();
    DropZone* GetHoveredDropZoneSafe(ImVec2 mousePos);
    
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
    std::shared_ptr<DockNode> FindNodeById(std::shared_ptr<DockNode> node, const std::string& id);
    std::shared_ptr<DockNode> FindNodeByPath(std::shared_ptr<DockNode> node, const std::string& path);
    void CleanupEmptyNodes();
    void CreateDefaultLayout();
    
    // Panel assignment persistence
    void SavePanelAssignments(const std::string& filename);
    void LoadPanelAssignments(const std::string& filename, 
                             const std::unordered_map<std::string, std::shared_ptr<Panel>>& panels);
    
    // Layout cloning for custom layouts (legacy)
    std::shared_ptr<DockNode> CloneNodeTree(std::shared_ptr<DockNode> node);
    void RestorePanelsToClonedTree(std::shared_ptr<DockNode> targetTree, 
                                  std::shared_ptr<DockNode> sourceTree,
                                  const std::unordered_map<std::string, std::shared_ptr<Panel>>& panels);
    
    // New persistent layout system
    PersistentLayoutInfo CaptureCurrentLayout(const std::string& layoutName) const;
    bool RestoreLayout(const PersistentLayoutInfo& layoutInfo);
    std::shared_ptr<DockNode> CreateNodeFromLayoutData(const LayoutNodeData& data);
    LayoutNodeData CaptureNodeData(std::shared_ptr<DockNode> node) const;
    
    // Constants
    static constexpr float TAB_HEIGHT = 25.0f;
    static constexpr float RESIZE_HANDLE_SIZE = 8.0f;  // Larger resize handles
    static constexpr float MIN_NODE_SIZE = 20.0f;      // Allow smaller panels
    static constexpr float DROP_ZONE_SIZE = 40.0f;
    static constexpr ImU32 DROP_ZONE_COLOR = IM_COL32(70, 130, 200, 100);
    static constexpr ImU32 DROP_ZONE_BORDER_COLOR = IM_COL32(70, 130, 200, 255);
    
    // Save dialog state
    bool m_showSaveDialog = false;
    char m_saveDialogBuffer[128] = {};
    
    // In-memory saved layouts for instant switching (legacy)
    std::unordered_map<std::string, std::function<void(DockingSystem*)>> m_savedLayouts;
    
    // New persistent layout storage
    std::unordered_map<std::string, PersistentLayoutInfo> m_persistentLayouts;
    
    // Available panels registry (includes both docked and closed panels)
    std::unordered_map<std::string, std::shared_ptr<Panel>> m_availablePanels;
    
    // Track current base layout for saving
    std::string m_currentBaseLayout = "Unity";
};

} // namespace BGE