#pragma once

#include "Panel.h"
#include "../../Core/EventBus.h"
#include "../../Core/AssetTypes.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <chrono>

namespace BGE {

// Asset info structure
struct AssetInfo {
    std::string path;
    std::string name;
    std::string extension;
    AssetType type;
    size_t fileSize;
    std::filesystem::file_time_type lastModified;
    bool isDirectory;
    
    // Cached data
    bool thumbnailLoaded = false;
    uint32_t thumbnailTexture = 0;
    std::string metadata;
};

// AssetSelectionChangedEvent is included from AssetTypes.h

class AssetBrowserPanel : public Panel {
public:
    AssetBrowserPanel(const std::string& name);
    ~AssetBrowserPanel();
    
    void Initialize() override;
    void OnRender() override;
    
    // Asset management
    const std::string& GetCurrentDirectory() const { return m_currentDirectory; }
    void NavigateToDirectory(const std::string& path);
    void RefreshCurrentDirectory();
    
private:
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    
    // UI Rendering
    void RenderToolbar();
    void RenderBreadcrumbs();
    void RenderLeftPanel();      // Directory tree
    void RenderMainPanel();      // Asset grid
    void RenderStatusBar();
    
    // Directory tree
    void RenderDirectoryTree(const std::string& path, int depth = 0);
    bool IsDirectoryExpanded(const std::string& path) const;
    void SetDirectoryExpanded(const std::string& path, bool expanded);
    
    // Asset grid
    void RenderAssetGrid();
    void RenderAssetIcon(const AssetInfo& asset, const ImVec2& iconSize);
    void RenderAssetContextMenu(const AssetInfo& asset);
    void RenderCreateContextMenu();
    
    // File system operations
    void ScanDirectory(const std::string& path);
    void RefreshAssets();
    bool CreateFolder(const std::string& name);
    bool CreateAsset(const std::string& name, AssetType type);
    bool DeleteAsset(const std::string& path);
    bool RenameAsset(const std::string& oldPath, const std::string& newName);
    bool DuplicateAsset(const std::string& path);
    bool MoveAsset(const std::string& srcPath, const std::string& dstDirectory);
    
    // Asset type detection
    AssetType GetAssetType(const std::string& path) const;
    std::string GetAssetIconText(AssetType type) const;
    
    // Thumbnail management
    void LoadThumbnail(AssetInfo& asset);
    uint32_t GetDefaultIcon(AssetType type) const;
    
    // Selection and events
    void SelectAsset(const std::string& path);
    void BroadcastSelectionChanged();
    
    // Drag and drop
    void HandleDragAndDrop();
    void BeginDragAsset(const AssetInfo& asset);
    
    // File system monitoring
    void StartFileSystemWatcher();
    void StopFileSystemWatcher();
    void CheckFileSystemChanges();
    
    // Project paths
    std::string m_projectRoot;
    std::string m_assetsDirectory;
    std::string m_currentDirectory;
    
    // UI state
    float m_leftPanelWidth = 200.0f;
    float m_iconSize = 64.0f;
    int m_gridColumns = 4;
    bool m_showHiddenFiles = false;
    
    // Assets and directories
    std::vector<AssetInfo> m_currentAssets;
    std::unordered_map<std::string, bool> m_expandedDirectories;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_directoryModTimes;
    
    // Selection
    std::string m_selectedAsset;
    std::vector<std::string> m_multiSelection;
    
    // Search and filter
    char m_searchBuffer[256] = "";
    AssetType m_filterType = AssetType::Unknown;
    
    // Context menu state
    bool m_showCreateMenu = false;
    bool m_showAssetMenu = false;
    std::string m_contextMenuAsset;
    
    // Rename state
    bool m_renameMode = false;
    char m_renameBuffer[256] = "";
    std::string m_renamingAsset;
    
    // File system monitoring
    std::chrono::steady_clock::time_point m_lastRefresh;
    static constexpr float REFRESH_INTERVAL = 2.0f; // seconds
    
    // Event bus
    EventBus* m_eventBus = nullptr;
    
    // Icon cache
    std::unordered_map<AssetType, uint32_t> m_iconCache;
};

} // namespace BGE