#pragma once

#include "IAsset.h"
#include "AssetHandle.h"
#include <memory>
#include <string>

namespace BGE {

// Base interface for asset loaders
class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;
    
    virtual std::shared_ptr<IAsset> LoadAsset(const std::string& filePath, const AssetHandle& handle) = 0;
    virtual bool CanLoadAsset(const std::string& filePath) const = 0;
    virtual AssetType GetAssetType() const = 0;
};

// Texture loader implementation
class TextureLoader : public IAssetLoader {
public:
    std::shared_ptr<IAsset> LoadAsset(const std::string& filePath, const AssetHandle& handle) override;
    bool CanLoadAsset(const std::string& filePath) const override;
    AssetType GetAssetType() const override { return AssetType::Texture; }
    
private:
    bool IsValidTextureExtension(const std::string& extension) const;
};

// Material loader implementation
class MaterialLoader : public IAssetLoader {
public:
    std::shared_ptr<IAsset> LoadAsset(const std::string& filePath, const AssetHandle& handle) override;
    bool CanLoadAsset(const std::string& filePath) const override;
    AssetType GetAssetType() const override { return AssetType::Material; }
    
private:
    bool IsMaterialFile(const std::string& filePath) const;
};

// Prefab loader implementation
class PrefabLoader : public IAssetLoader {
public:
    std::shared_ptr<IAsset> LoadAsset(const std::string& filePath, const AssetHandle& handle) override;
    bool CanLoadAsset(const std::string& filePath) const override;
    AssetType GetAssetType() const override { return AssetType::Prefab; }
};

// Scene loader implementation
class SceneLoader : public IAssetLoader {
public:
    std::shared_ptr<IAsset> LoadAsset(const std::string& filePath, const AssetHandle& handle) override;
    bool CanLoadAsset(const std::string& filePath) const override;
    AssetType GetAssetType() const override { return AssetType::Scene; }
    
private:
    bool IsSceneFile(const std::string& filePath) const;
};

} // namespace BGE