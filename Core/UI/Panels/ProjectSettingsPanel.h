#pragma once

#include "../Framework/Panel.h"
#include "../../EventBus.h"
#include "../../AssetTypes.h"
#include "../IconManager.h"
#include "../../../AssetPipeline/AssetHandle.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace BGE {

// Project settings for asset type thumbnails
struct AssetTypeThumbnailSettings {
    std::string customThumbnailPath;
    uint32_t thumbnailTextureId = 0;
    bool useCustomThumbnail = false;
};

// Event for when project settings change
struct ProjectSettingsChangedEvent {
    std::string settingName;
    
    ProjectSettingsChangedEvent(const std::string& name) : settingName(name) {}
};

// Settings sections/chapters
enum class SettingsSection {
    General,
    AssetThumbnails,
    Rendering,
    Input,
    Audio,
    Performance,
    HierarchyView
};

class ProjectSettingsPanel {
public:
    ProjectSettingsPanel(const std::string& name);
    ~ProjectSettingsPanel();
    
    void Initialize();
    void Render();
    
    // Window management
    void Show() { m_isVisible = true; }
    void Hide() { m_isVisible = false; }
    void Toggle() { m_isVisible = !m_isVisible; }
    bool IsVisible() const { return m_isVisible; }
    
    // Settings management
    void LoadProjectSettings();
    void SaveProjectSettings();
    void ResetToDefaults();
    
    // Asset type thumbnail management
    void SetAssetTypeThumbnail(AssetType type, const std::string& imagePath);
    void RemoveAssetTypeThumbnail(AssetType type);
    std::string GetAssetTypeThumbnailPath(AssetType type) const;
    uint32_t GetAssetTypeThumbnailTexture(AssetType type) const;
    bool HasCustomThumbnail(AssetType type) const;
    
    // Individual asset thumbnail management
    void SetIndividualAssetThumbnail(const AssetHandle& assetHandle, const std::string& assetPath, uint32_t textureId, const std::string& sourcePath);
    void RemoveIndividualAssetThumbnail(const AssetHandle& assetHandle);
    void RemoveIndividualAssetThumbnailByPath(const std::string& assetPath);
    uint32_t GetIndividualAssetThumbnail(const AssetHandle& assetHandle) const;
    uint32_t GetIndividualAssetThumbnailByPath(const std::string& assetPath) const;
    void UpdateAssetThumbnailMapping(const AssetHandle& oldHandle, const AssetHandle& newHandle, const std::string& newPath);
    void RestoreThumbnailFromPath(const AssetHandle& assetHandle, const std::string& assetPath);

private:
    // Event handling
    void RegisterEventListeners();
    void UnregisterEventListeners();
    
    // UI Rendering
    void RenderSectionsList();
    void RenderSettingsContent();
    void RenderGeneralSettings();
    void RenderAssetThumbnailsSettings();
    void RenderRenderingSettings();
    void RenderInputSettings();
    void RenderAudioSettings();
    void RenderPerformanceSettings();
    void RenderHierarchyViewSettings();
    void RenderAssetTypeRow(AssetType type, const char* typeName);
    
    // Hierarchy view settings
    bool IsShowingHierarchyIcons() const { return m_showHierarchyIcons; }
    bool IsShowingHierarchyVisibilityToggles() const { return m_showHierarchyVisibilityToggles; }
    bool IsShowingHierarchyLockToggles() const { return m_showHierarchyLockToggles; }
    
    // File operations
    void OpenThumbnailFileDialog(AssetType type);
    std::string OpenNativeFileDialog();
    bool LoadThumbnailTexture(AssetType type, const std::string& imagePath);
    std::string GetProjectSettingsPath() const;
    
    // Asset type utilities
    const char* GetAssetTypeName(AssetType type) const;
    const char* GetSectionName(SettingsSection section) const;
    const char* GetSectionIcon(SettingsSection section) const;
    std::vector<AssetType> GetAllAssetTypes() const;
    std::vector<SettingsSection> GetAllSections() const;
    
    // Individual asset thumbnail info
    struct IndividualAssetThumbnail {
        uint32_t textureId = 0;
        std::string sourcePath;
        std::string assetPath; // Store path for backup lookup
    };
    
    // State
    std::unordered_map<AssetType, AssetTypeThumbnailSettings> m_assetTypeThumbnails;
    std::unordered_map<AssetHandle, IndividualAssetThumbnail, AssetHandleHash> m_individualAssetThumbnails; // Per-asset thumbnails
    std::unordered_map<std::string, IndividualAssetThumbnail> m_pathBasedThumbnails; // Path-based backup for moved/recreated assets
    std::string m_projectRoot;
    std::string m_projectSettingsFile;
    std::string m_windowName;
    
    // UI state
    bool m_isVisible = false;
    SettingsSection m_currentSection = SettingsSection::General;
    AssetType m_fileDialogAssetType = AssetType::Unknown;
    float m_sectionListWidth = 200.0f;
    
    // Save feedback
    bool m_showSaveMessage = false;
    float m_saveMessageTimer = 0.0f;
    
    // Hierarchy view settings
    bool m_showHierarchyIcons = true;
    bool m_showHierarchyVisibilityToggles = true;
    bool m_showHierarchyLockToggles = true;
    
    // Services
    EventBus* m_eventBus = nullptr;
    IconManager* m_iconManager = nullptr;
};

} // namespace BGE