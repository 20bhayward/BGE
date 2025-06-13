#pragma once

#include <string>
#include <unordered_map>
#include <memory> // For std::shared_ptr
#include <filesystem> // For std::filesystem::file_time_type

#include "../Renderer/Texture.h" // Include the new Texture struct
#include "../Core/ServiceLocator.h"
#include "../Renderer/Renderer.h"
#include "../Core/Logger.h"


namespace BGE {

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;
    
    bool Initialize() { return true; }
    void Shutdown() {}
    
    // TODO: Implement asset management functionality
    bool LoadAsset(const char* filename) { (void)filename; return true; }
    void UnloadAsset(const char* filename) { (void)filename; }

    std::shared_ptr<Texture> LoadTexture(const std::string& path);
    void Update();

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
};

} // namespace BGE