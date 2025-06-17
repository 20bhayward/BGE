#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>

#include "AssetHandle.h"
#include "IAsset.h"
#include "IAssetLoader.h"
#include "AssetRegistry.h"
#include "../Core/AssetTypes.h"
#include "../Core/EventBus.h"

namespace BGE {

// Asset reload event for hot-reloading
struct AssetReloadedEvent {
    AssetHandle handle;
    AssetType type;
    std::string path;
    
    AssetReloadedEvent(const AssetHandle& h, AssetType t, const std::string& p) 
        : handle(h), type(t), path(p) {}
};

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;
    
    bool Initialize(const std::string& assetsDirectory = "Assets");
    void Shutdown();
    
    // Generic asset loading
    template<typename T>
    std::shared_ptr<T> GetAsset(const AssetHandle& handle);
    
    std::shared_ptr<IAsset> GetAsset(const AssetHandle& handle);
    AssetHandle LoadAsset(const std::string& filePath);
    void UnloadAsset(const AssetHandle& handle);
    void ReloadAsset(const AssetHandle& handle);
    
    // Asset type specific getters
    std::shared_ptr<TextureAsset> GetTexture(const AssetHandle& handle);
    std::shared_ptr<MaterialAsset> GetMaterial(const AssetHandle& handle);
    std::shared_ptr<PrefabAsset> GetPrefab(const AssetHandle& handle);
    std::shared_ptr<SceneAsset> GetScene(const AssetHandle& handle);
    
    // Loader registration
    void RegisterLoader(std::unique_ptr<IAssetLoader> loader);
    
    // Asset registry access
    AssetRegistry& GetRegistry() { return m_registry; }
    const AssetRegistry& GetRegistry() const { return m_registry; }
    
    // File system monitoring
    void Update();
    void RefreshAssets();
    
    // Legacy texture loading for compatibility
    std::shared_ptr<TextureAsset> LoadTexture(const std::string& path);

private:
    IAssetLoader* GetLoaderForAsset(const std::string& filePath) const;
    void BroadcastAssetReloaded(const AssetHandle& handle, const std::string& path);
    
    AssetRegistry m_registry;
    std::unordered_map<AssetHandle, std::shared_ptr<IAsset>, AssetHandleHash> m_assetCache;
    std::unordered_map<AssetType, std::unique_ptr<IAssetLoader>> m_loaders;
    
    EventBus* m_eventBus = nullptr;
    std::string m_assetsDirectory;
};

// Template implementation
template<typename T>
std::shared_ptr<T> AssetManager::GetAsset(const AssetHandle& handle) {
    return std::dynamic_pointer_cast<T>(GetAsset(handle));
}

} // namespace BGE