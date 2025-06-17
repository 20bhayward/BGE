#include "AssetManager.h"
#include "../Core/ServiceLocator.h"
#include <iostream>

// Define STB_IMAGE_IMPLEMENTATION once for the entire AssetPipeline
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4505) // unreferenced function with internal linkage has been removed
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/stb/stb_image.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace BGE {

bool AssetManager::Initialize(const std::string& assetsDirectory) {
    m_assetsDirectory = assetsDirectory;
    m_eventBus = ServiceLocator::Instance().GetService<EventBus>().get();
    
    // Initialize the asset registry
    if (!m_registry.Initialize(assetsDirectory)) {
        std::cerr << "Failed to initialize AssetRegistry" << std::endl;
        return false;
    }
    
    // Register default loaders
    RegisterLoader(std::make_unique<TextureLoader>());
    RegisterLoader(std::make_unique<MaterialLoader>());
    RegisterLoader(std::make_unique<PrefabLoader>());
    RegisterLoader(std::make_unique<SceneLoader>());
    
    return true;
}

void AssetManager::Shutdown() {
    m_assetCache.clear();
    m_loaders.clear();
    m_registry.Shutdown();
    m_eventBus = nullptr;
}

std::shared_ptr<IAsset> AssetManager::GetAsset(const AssetHandle& handle) {
    if (!handle.IsValid()) {
        return nullptr;
    }
    
    // Check cache first
    auto it = m_assetCache.find(handle);
    if (it != m_assetCache.end()) {
        return it->second;
    }
    
    // Load asset if not in cache
    std::string path = m_registry.GetAssetPath(handle);
    if (path.empty()) {
        return nullptr;
    }
    
    IAssetLoader* loader = GetLoaderForAsset(path);
    if (!loader) {
        std::cerr << "No loader found for asset: " << path << std::endl;
        return nullptr;
    }
    
    auto asset = loader->LoadAsset(path, handle);
    if (asset) {
        m_assetCache[handle] = asset;
    }
    
    return asset;
}

AssetHandle AssetManager::LoadAsset(const std::string& filePath) {
    AssetHandle handle = m_registry.RegisterAsset(filePath);
    if (handle.IsValid()) {
        // Pre-load the asset into cache
        GetAsset(handle);
    }
    return handle;
}

void AssetManager::UnloadAsset(const AssetHandle& handle) {
    m_assetCache.erase(handle);
    m_registry.UnregisterAsset(handle);
}

void AssetManager::ReloadAsset(const AssetHandle& handle) {
    std::string path = m_registry.GetAssetPath(handle);
    if (path.empty()) {
        return;
    }
    
    // Remove from cache to force reload
    m_assetCache.erase(handle);
    
    // Refresh in registry
    m_registry.RefreshAsset(path);
    
    // Reload the asset
    auto asset = GetAsset(handle);
    if (asset) {
        BroadcastAssetReloaded(handle, path);
    }
}

std::shared_ptr<TextureAsset> AssetManager::GetTexture(const AssetHandle& handle) {
    return GetAsset<TextureAsset>(handle);
}

std::shared_ptr<MaterialAsset> AssetManager::GetMaterial(const AssetHandle& handle) {
    return GetAsset<MaterialAsset>(handle);
}

std::shared_ptr<PrefabAsset> AssetManager::GetPrefab(const AssetHandle& handle) {
    return GetAsset<PrefabAsset>(handle);
}

std::shared_ptr<SceneAsset> AssetManager::GetScene(const AssetHandle& handle) {
    return GetAsset<SceneAsset>(handle);
}

void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader> loader) {
    AssetType type = loader->GetAssetType();
    m_loaders[type] = std::move(loader);
}

void AssetManager::Update() {
    // Check for file system changes and reload modified assets
    for (const auto& assetPair : m_registry.GetAllAssets()) {
        const AssetHandle& handle = assetPair.first;
        const AssetMetadata& metadata = assetPair.second;
        
        std::string path = m_registry.GetAssetPath(handle);
        if (path.empty() || !std::filesystem::exists(path)) {
            continue;
        }
        
        try {
            auto currentModTime = std::filesystem::last_write_time(path);
            if (currentModTime > metadata.lastModified) {
                ReloadAsset(handle);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error checking file time for " << path << ": " << e.what() << std::endl;
        }
    }
}

void AssetManager::RefreshAssets() {
    m_registry.ScanAssetsDirectory();
}

std::shared_ptr<TextureAsset> AssetManager::LoadTexture(const std::string& path) {
    AssetHandle handle = LoadAsset(path);
    return GetTexture(handle);
}

IAssetLoader* AssetManager::GetLoaderForAsset(const std::string& filePath) const {
    for (const auto& loaderPair : m_loaders) {
        if (loaderPair.second->CanLoadAsset(filePath)) {
            return loaderPair.second.get();
        }
    }
    return nullptr;
}

void AssetManager::BroadcastAssetReloaded(const AssetHandle& handle, const std::string& path) {
    if (m_eventBus) {
        AssetType type = m_registry.GetAssetType(handle);
        m_eventBus->Publish(AssetReloadedEvent(handle, type, path));
    }
}

} // namespace BGE