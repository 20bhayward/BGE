#pragma once

#include "AssetHandle.h"
#include "../Core/AssetTypes.h"
#include <string>
#include <filesystem>

namespace BGE {

// Base interface for all assets
class IAsset {
public:
    virtual ~IAsset() = default;
    
    AssetHandle GetHandle() const { return m_handle; }
    const std::string& GetPath() const { return m_path; }
    AssetType GetType() const { return m_type; }
    std::filesystem::file_time_type GetLastModified() const { return m_lastModified; }
    
    void SetHandle(const AssetHandle& handle) { m_handle = handle; }
    void SetPath(const std::string& path) { m_path = path; }
    void SetLastModified(std::filesystem::file_time_type time) { m_lastModified = time; }

protected:
    IAsset(AssetType type) : m_type(type) {}
    
    AssetHandle m_handle;
    std::string m_path;
    AssetType m_type;
    std::filesystem::file_time_type m_lastModified;
};

// Concrete asset types
class TextureAsset : public IAsset {
public:
    TextureAsset() : IAsset(AssetType::Texture) {}
    
    int width = 0;
    int height = 0;
    int channels = 0;
    uint32_t rendererId = 0;
};

class MaterialAsset : public IAsset {
public:
    MaterialAsset() : IAsset(AssetType::Material) {}
    
    struct MaterialData {
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float roughness = 0.5f;
        float metallic = 0.0f;
        float emission = 0.0f;
        AssetHandle albedoTexture;
        AssetHandle normalTexture;
        AssetHandle roughnessTexture;
    } data;
};

class PrefabAsset : public IAsset {
public:
    PrefabAsset() : IAsset(AssetType::Prefab) {}
    
    std::string entityData; // JSON serialized entity data
};

class SceneAsset : public IAsset {
public:
    SceneAsset() : IAsset(AssetType::Scene) {}
    
    std::string sceneData; // JSON serialized scene data
};

} // namespace BGE