#include "AssetRegistry.h"
#include "../ThirdParty/json/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace BGE {

bool AssetRegistry::Initialize(const std::string& assetsDirectory) {
    m_assetsDirectory = std::filesystem::absolute(assetsDirectory).string();
    
    if (!std::filesystem::exists(m_assetsDirectory)) {
        std::filesystem::create_directories(m_assetsDirectory);
    }
    
    ScanAssetsDirectory();
    return true;
}

void AssetRegistry::Shutdown() {
    m_assets.clear();
    m_pathToHandle.clear();
    m_dependencies.clear();
    m_dependents.clear();
}

AssetHandle AssetRegistry::RegisterAsset(const std::string& filePath) {
    std::string absolutePath = std::filesystem::absolute(filePath).string();
    
    if (!IsInAssetsDirectory(absolutePath)) {
        return AssetHandle();
    }
    
    auto it = m_pathToHandle.find(absolutePath);
    if (it != m_pathToHandle.end()) {
        return it->second;
    }
    
    AssetMetadata metadata;
    
    if (!LoadMetaFile(absolutePath, metadata)) {
        CreateMetaFile(absolutePath, metadata);
    }
    
    metadata.lastModified = std::filesystem::last_write_time(absolutePath);
    m_assets[metadata.handle] = metadata;
    m_pathToHandle[absolutePath] = metadata.handle;
    
    return metadata.handle;
}

void AssetRegistry::UnregisterAsset(const AssetHandle& handle) {
    auto it = m_assets.find(handle);
    if (it != m_assets.end()) {
        std::string path = GetAssetPath(handle);
        m_pathToHandle.erase(path);
        m_assets.erase(it);
        
        m_dependencies.erase(handle);
        m_dependents.erase(handle);
    }
}

void AssetRegistry::UnregisterAsset(const std::string& filePath) {
    AssetHandle handle = GetAssetHandle(filePath);
    if (handle.IsValid()) {
        UnregisterAsset(handle);
    }
}

bool AssetRegistry::HasAsset(const AssetHandle& handle) const {
    return m_assets.find(handle) != m_assets.end();
}

bool AssetRegistry::HasAsset(const std::string& filePath) const {
    std::string absolutePath = std::filesystem::absolute(filePath).string();
    return m_pathToHandle.find(absolutePath) != m_pathToHandle.end();
}

AssetHandle AssetRegistry::GetAssetHandle(const std::string& filePath) const {
    std::string absolutePath = std::filesystem::absolute(filePath).string();
    auto it = m_pathToHandle.find(absolutePath);
    return it != m_pathToHandle.end() ? it->second : AssetHandle();
}

std::string AssetRegistry::GetAssetPath(const AssetHandle& handle) const {
    for (const auto& pair : m_pathToHandle) {
        if (pair.second == handle) {
            return pair.first;
        }
    }
    return "";
}

AssetMetadata AssetRegistry::GetAssetMetadata(const AssetHandle& handle) const {
    auto it = m_assets.find(handle);
    return it != m_assets.end() ? it->second : AssetMetadata();
}

AssetType AssetRegistry::GetAssetType(const AssetHandle& handle) const {
    auto it = m_assets.find(handle);
    return it != m_assets.end() ? it->second.type : AssetType::Unknown;
}

void AssetRegistry::ScanAssetsDirectory() {
    if (!std::filesystem::exists(m_assetsDirectory)) {
        return;
    }
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_assetsDirectory)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            
            if (path.size() >= 5 && path.substr(path.size() - 5) == ".meta") {
                continue;
            }
            
            RegisterAsset(path);
        }
    }
}

void AssetRegistry::RefreshAsset(const std::string& filePath) {
    std::string absolutePath = std::filesystem::absolute(filePath).string();
    
    if (!std::filesystem::exists(absolutePath)) {
        UnregisterAsset(absolutePath);
        return;
    }
    
    auto currentModTime = std::filesystem::last_write_time(absolutePath);
    AssetHandle handle = GetAssetHandle(absolutePath);
    
    if (handle.IsValid()) {
        auto& metadata = m_assets[handle];
        metadata.lastModified = currentModTime;
    } else {
        RegisterAsset(absolutePath);
    }
}

std::string AssetRegistry::GetMetaFilePath(const std::string& assetPath) const {
    return assetPath + ".meta";
}

bool AssetRegistry::LoadMetaFile(const std::string& assetPath, AssetMetadata& metadata) const {
    std::string metaPath = GetMetaFilePath(assetPath);
    
    if (!std::filesystem::exists(metaPath)) {
        return false;
    }
    
    try {
        std::ifstream file(metaPath);
        json j;
        file >> j;
        
        metadata.handle = AssetHandle::FromString(j["uuid"].get<std::string>());
        metadata.type = static_cast<AssetType>(j["type"].get<int>());
        metadata.version = j.value("version", 1);
        metadata.importerSettings = j.value("importerSettings", "{}");
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading meta file " << metaPath << ": " << e.what() << std::endl;
        return false;
    }
}

bool AssetRegistry::SaveMetaFile(const std::string& assetPath, const AssetMetadata& metadata) const {
    std::string metaPath = GetMetaFilePath(assetPath);
    
    try {
        json j;
        j["uuid"] = metadata.handle.ToString();
        j["type"] = static_cast<int>(metadata.type);
        j["version"] = metadata.version;
        j["importerSettings"] = json::parse(metadata.importerSettings);
        
        std::ofstream file(metaPath);
        file << j.dump(4);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving meta file " << metaPath << ": " << e.what() << std::endl;
        return false;
    }
}

void AssetRegistry::CreateMetaFile(const std::string& assetPath, AssetMetadata& metadata) {
    metadata.handle = AssetHandle::Generate();
    metadata.type = DetectAssetType(assetPath);
    metadata.version = 1;
    metadata.importerSettings = "{}";
    
    SaveMetaFile(assetPath, metadata);
}

AssetType AssetRegistry::DetectAssetType(const std::string& filePath) const {
    std::string ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        return AssetType::Texture;
    } else if (ext == ".json") {
        std::string filename = std::filesystem::path(filePath).filename().string();
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        
        if (filename.find("material") != std::string::npos) {
            return AssetType::Material;
        } else if (filename.find("scene") != std::string::npos) {
            return AssetType::Scene;
        }
    } else if (ext == ".bprefab") {
        return AssetType::Prefab;
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        return AssetType::Audio;
    } else if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
        return AssetType::Script;
    } else if (ext == ".obj" || ext == ".fbx") {
        return AssetType::Model;
    } else if (ext == ".anim") {
        return AssetType::Animation;
    }
    
    return AssetType::Unknown;
}

bool AssetRegistry::IsInAssetsDirectory(const std::string& path) const {
    std::string relativePath = std::filesystem::relative(path, m_assetsDirectory).string();
    return !relativePath.starts_with("..");
}

std::string AssetRegistry::GetRelativePath(const std::string& path) const {
    return std::filesystem::relative(path, m_assetsDirectory).string();
}

void AssetRegistry::AddDependency(const AssetHandle& asset, const AssetHandle& dependency) {
    m_dependencies[asset].push_back(dependency);
    m_dependents[dependency].push_back(asset);
}

void AssetRegistry::RemoveDependency(const AssetHandle& asset, const AssetHandle& dependency) {
    auto& deps = m_dependencies[asset];
    deps.erase(std::remove(deps.begin(), deps.end(), dependency), deps.end());
    
    auto& dependents = m_dependents[dependency];
    dependents.erase(std::remove(dependents.begin(), dependents.end(), asset), dependents.end());
}

std::vector<AssetHandle> AssetRegistry::GetDependencies(const AssetHandle& asset) const {
    auto it = m_dependencies.find(asset);
    return it != m_dependencies.end() ? it->second : std::vector<AssetHandle>();
}

std::vector<AssetHandle> AssetRegistry::GetDependents(const AssetHandle& asset) const {
    auto it = m_dependents.find(asset);
    return it != m_dependents.end() ? it->second : std::vector<AssetHandle>();
}

} // namespace BGE