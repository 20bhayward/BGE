#pragma once

#include "AssetHandle.h"
#include "../Core/AssetTypes.h"
#include <string>
#include <unordered_map>
#include <filesystem>

namespace BGE {

// Asset metadata stored in .meta files
struct AssetMetadata {
    AssetHandle handle;
    AssetType type = AssetType::Unknown;
    int version = 1;
    std::string importerSettings; // JSON string for importer-specific settings
    std::filesystem::file_time_type lastModified;
};

// Manages asset registration and .meta file generation
class AssetRegistry {
public:
    AssetRegistry() = default;
    ~AssetRegistry() = default;
    
    bool Initialize(const std::string& assetsDirectory);
    void Shutdown();
    
    // Asset registration
    AssetHandle RegisterAsset(const std::string& filePath);
    void UnregisterAsset(const AssetHandle& handle);
    void UnregisterAsset(const std::string& filePath);
    
    // Asset lookup
    bool HasAsset(const AssetHandle& handle) const;
    bool HasAsset(const std::string& filePath) const;
    AssetHandle GetAssetHandle(const std::string& filePath) const;
    std::string GetAssetPath(const AssetHandle& handle) const;
    AssetMetadata GetAssetMetadata(const AssetHandle& handle) const;
    AssetType GetAssetType(const AssetHandle& handle) const;
    
    // Asset scanning and monitoring
    void ScanAssetsDirectory();
    void RefreshAsset(const std::string& filePath);
    
    // Dependency tracking
    void AddDependency(const AssetHandle& asset, const AssetHandle& dependency);
    void RemoveDependency(const AssetHandle& asset, const AssetHandle& dependency);
    std::vector<AssetHandle> GetDependencies(const AssetHandle& asset) const;
    std::vector<AssetHandle> GetDependents(const AssetHandle& asset) const;
    
    // Iterator support
    const std::unordered_map<AssetHandle, AssetMetadata, AssetHandleHash>& GetAllAssets() const { return m_assets; }

private:
    // .meta file operations
    std::string GetMetaFilePath(const std::string& assetPath) const;
    bool LoadMetaFile(const std::string& assetPath, AssetMetadata& metadata) const;
    bool SaveMetaFile(const std::string& assetPath, const AssetMetadata& metadata) const;
    void CreateMetaFile(const std::string& assetPath, AssetMetadata& metadata);
    
    // Asset type detection
    AssetType DetectAssetType(const std::string& filePath) const;
    
    // Path utilities
    bool IsInAssetsDirectory(const std::string& path) const;
    std::string GetRelativePath(const std::string& path) const;
    
    std::string m_assetsDirectory;
    std::unordered_map<AssetHandle, AssetMetadata, AssetHandleHash> m_assets;
    std::unordered_map<std::string, AssetHandle> m_pathToHandle;
    
    // Dependency tracking
    std::unordered_map<AssetHandle, std::vector<AssetHandle>, AssetHandleHash> m_dependencies;
    std::unordered_map<AssetHandle, std::vector<AssetHandle>, AssetHandleHash> m_dependents;
};

} // namespace BGE