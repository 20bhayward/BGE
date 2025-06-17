#include "IAssetLoader.h"
#include "../Core/ServiceLocator.h"
#include "../Renderer/Renderer.h"
#include "../ThirdParty/json/json.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

// STB Image for texture loading
#include "../ThirdParty/stb/stb_image.h"

using json = nlohmann::json;

namespace BGE {

// TextureLoader Implementation
std::shared_ptr<IAsset> TextureLoader::LoadAsset(const std::string& filePath, const AssetHandle& handle) {
    if (!CanLoadAsset(filePath)) {
        return nullptr;
    }
    
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Error loading texture: " << filePath << " - " << stbi_failure_reason() << std::endl;
        return nullptr;
    }
    
    auto renderer = ServiceLocator::Instance().GetService<Renderer>();
    uint32_t textureId = 0;
    
    if (renderer) {
        textureId = renderer->CreateTexture(width, height, channels, data);
    } else {
        std::cerr << "Renderer service not found. Cannot create GPU texture for: " << filePath << std::endl;
        stbi_image_free(data);
        return nullptr;
    }
    
    auto textureAsset = std::make_shared<TextureAsset>();
    textureAsset->SetHandle(handle);
    textureAsset->SetPath(filePath);
    textureAsset->SetLastModified(std::filesystem::last_write_time(filePath));
    textureAsset->width = width;
    textureAsset->height = height;
    textureAsset->channels = channels;
    textureAsset->rendererId = textureId;
    
    stbi_image_free(data);
    return textureAsset;
}

bool TextureLoader::CanLoadAsset(const std::string& filePath) const {
    std::string ext = std::filesystem::path(filePath).extension().string();
    return IsValidTextureExtension(ext);
}

bool TextureLoader::IsValidTextureExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || 
           ext == ".bmp" || ext == ".tga";
}

// MaterialLoader Implementation
std::shared_ptr<IAsset> MaterialLoader::LoadAsset(const std::string& filePath, const AssetHandle& handle) {
    if (!CanLoadAsset(filePath)) {
        return nullptr;
    }
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Could not open material file: " << filePath << std::endl;
            return nullptr;
        }
        
        json j;
        file >> j;
        
        auto materialAsset = std::make_shared<MaterialAsset>();
        materialAsset->SetHandle(handle);
        materialAsset->SetPath(filePath);
        materialAsset->SetLastModified(std::filesystem::last_write_time(filePath));
        
        // Load material properties with defaults
        if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3) {
            materialAsset->data.color[0] = j["color"][0].get<float>();
            materialAsset->data.color[1] = j["color"][1].get<float>();
            materialAsset->data.color[2] = j["color"][2].get<float>();
            if (j["color"].size() >= 4) {
                materialAsset->data.color[3] = j["color"][3].get<float>();
            }
        }
        
        materialAsset->data.roughness = j.value("roughness", 0.5f);
        materialAsset->data.metallic = j.value("metallic", 0.0f);
        materialAsset->data.emission = j.value("emission", 0.0f);
        
        // Load texture references (if any)
        if (j.contains("albedoTexture") && !j["albedoTexture"].get<std::string>().empty()) {
            materialAsset->data.albedoTexture = AssetHandle::FromString(j["albedoTexture"].get<std::string>());
        }
        
        if (j.contains("normalTexture") && !j["normalTexture"].get<std::string>().empty()) {
            materialAsset->data.normalTexture = AssetHandle::FromString(j["normalTexture"].get<std::string>());
        }
        
        if (j.contains("roughnessTexture") && !j["roughnessTexture"].get<std::string>().empty()) {
            materialAsset->data.roughnessTexture = AssetHandle::FromString(j["roughnessTexture"].get<std::string>());
        }
        
        return materialAsset;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading material " << filePath << ": " << e.what() << std::endl;
        return nullptr;
    }
}

bool MaterialLoader::CanLoadAsset(const std::string& filePath) const {
    return IsMaterialFile(filePath);
}

bool MaterialLoader::IsMaterialFile(const std::string& filePath) const {
    std::string ext = std::filesystem::path(filePath).extension().string();
    if (ext != ".json") {
        return false;
    }
    
    std::string filename = std::filesystem::path(filePath).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    return filename.find("material") != std::string::npos;
}

// PrefabLoader Implementation
std::shared_ptr<IAsset> PrefabLoader::LoadAsset(const std::string& filePath, const AssetHandle& handle) {
    if (!CanLoadAsset(filePath)) {
        return nullptr;
    }
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Could not open prefab file: " << filePath << std::endl;
            return nullptr;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        auto prefabAsset = std::make_shared<PrefabAsset>();
        prefabAsset->SetHandle(handle);
        prefabAsset->SetPath(filePath);
        prefabAsset->SetLastModified(std::filesystem::last_write_time(filePath));
        prefabAsset->entityData = content;
        
        return prefabAsset;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading prefab " << filePath << ": " << e.what() << std::endl;
        return nullptr;
    }
}

bool PrefabLoader::CanLoadAsset(const std::string& filePath) const {
    std::string ext = std::filesystem::path(filePath).extension().string();
    return ext == ".bprefab";
}

// SceneLoader Implementation
std::shared_ptr<IAsset> SceneLoader::LoadAsset(const std::string& filePath, const AssetHandle& handle) {
    if (!CanLoadAsset(filePath)) {
        return nullptr;
    }
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Could not open scene file: " << filePath << std::endl;
            return nullptr;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        auto sceneAsset = std::make_shared<SceneAsset>();
        sceneAsset->SetHandle(handle);
        sceneAsset->SetPath(filePath);
        sceneAsset->SetLastModified(std::filesystem::last_write_time(filePath));
        sceneAsset->sceneData = content;
        
        return sceneAsset;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading scene " << filePath << ": " << e.what() << std::endl;
        return nullptr;
    }
}

bool SceneLoader::CanLoadAsset(const std::string& filePath) const {
    return IsSceneFile(filePath);
}

bool SceneLoader::IsSceneFile(const std::string& filePath) const {
    std::string ext = std::filesystem::path(filePath).extension().string();
    if (ext != ".json") {
        return false;
    }
    
    std::string filename = std::filesystem::path(filePath).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    return filename.find("scene") != std::string::npos;
}

} // namespace BGE