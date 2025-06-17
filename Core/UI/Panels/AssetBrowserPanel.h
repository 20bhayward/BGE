#pragma once

#include "../Framework/Panel.h"
#include "../../EventBus.h"
#include "../../AssetTypes.h"
#include "../../../AssetPipeline/AssetHandle.h"
#include "../../../AssetPipeline/AssetManager.h"
#include "../IconManager.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <chrono>

namespace BGE {

// Asset info using the new asset system
struct AssetInfo {
    std::string path;
    std::string name;
    std::string extension;
    AssetType type;
    AssetHandle handle;
    size_t fileSize;
    std::filesystem::file_time_type lastModified;
    bool isDirectory;
    
    // Cached data
    std::string metadata;
};

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
    void OnAssetReloaded(const AssetReloadedEvent& event);
    
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
    void RenderAssetContextMenu();
    void RenderAssetMenuContent();
    void RenderCreateContextMenu();
    
    // File system operations
    void ScanDirectory(const std::string& path);
    void RefreshAssets();
    bool CreateFolder(const std::string& name);
    bool CreateAsset(const std::string& name, AssetType type);
    bool DeleteAsset(const std::string& path);
    bool DeleteAssets(const std::vector<std::string>& paths);
    bool RenameAsset(const std::string& oldPath, const std::string& newName);
    bool DuplicateAsset(const std::string& path);
    bool MoveAsset(const std::string& srcPath, const std::string& dstDirectory);
    
    // Professional asset management
    std::string GenerateUniqueAssetName(const std::string& baseName, const std::string& directory = "");
    std::string GenerateUniqueAssetNameGlobally(const std::string& baseName);
    bool ValidateAssetName(const std::string& name, bool isDirectory = false);
    void NotifyAssetSystemOfChanges(const std::string& assetPath, const std::string& operation);
    
    // Clipboard operations
    void CopyAssets(const std::vector<std::string>& assetPaths);
    void CutAssets(const std::vector<std::string>& assetPaths);
    void PasteAssets(const std::string& destinationDirectory);
    bool CanPaste() const;
    void ClearClipboard();
    
    // Asset type detection
    AssetType GetAssetType(const std::string& path) const;
    
    // Icon management
    uint32_t GetAssetIcon(AssetType type) const;
    
    // Selection and events
    void SelectAsset(const std::string& path);
    void BroadcastSelectionChanged();
    
    // Drag and drop
    void HandleDragAndDrop();
    void BeginDragAsset(const AssetInfo& asset);
    
    // File system monitoring
    void CheckFileSystemChanges();
    
    // Input handling
    void HandleKeyboardShortcuts();
    
    // Project paths
    std::string m_projectRoot;
    std::string m_assetsDirectory;
    std::string m_currentDirectory;
    
    // UI state
    float m_leftPanelWidth = 200.0f;
    float m_iconSize = 64.0f;
    int m_gridColumns = 4;
    bool m_showHiddenFiles = false;
    bool m_showMetaFiles = false;
    
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
    std::vector<std::string> m_selectedAssetsForMenu;
    
    // Rename state
    bool m_renameMode = false;
    char m_renameBuffer[256] = "";
    std::string m_renamingAsset;
    bool m_renameInputWasActive = false;
    
    // Drag and drop state
    bool m_isDragging = false;
    std::string m_draggedAsset;
    AssetInfo m_draggedAssetInfo;
    
    // Clipboard system
    enum class ClipboardOperation { None, Copy, Cut };
    ClipboardOperation m_clipboardOperation = ClipboardOperation::None;
    std::vector<std::string> m_clipboardAssets;
    
    // File system monitoring
    std::chrono::steady_clock::time_point m_lastRefresh;
    static constexpr float REFRESH_INTERVAL = 2.0f; // seconds
    
    // Services
    EventBus* m_eventBus = nullptr;
    AssetManager* m_assetManager = nullptr;
    IconManager* m_iconManager = nullptr;
};

} // namespace BGE