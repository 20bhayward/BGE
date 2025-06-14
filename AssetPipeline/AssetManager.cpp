#include "AssetManager.h"
#include <iostream> // For placeholder logging

// Suppress MSVC warnings for STB
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

// Minimal implementation - to be expanded later

std::shared_ptr<BGE::Texture> BGE::AssetManager::LoadTexture(const std::string& path) {
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end()) {
        return it->second;
    }

    int width, height, channels;
    // Correctly call stbi_load from the global namespace or ensure it's properly namespaced if applicable
    unsigned char* data = ::stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data) {
        // It's better to use the engine's logger if available
        // BGE_LOG_ERROR("AssetManager", "Error loading texture: " + path + " STBI: " + stbi_failure_reason());
        std::cerr << "Error loading texture: " << path << std::endl;
        return nullptr;
    }

    auto renderer = BGE::ServiceLocator::Instance().GetService<BGE::Renderer>();
    uint32_t textureId = 0;

    if (renderer) {
        textureId = renderer->CreateTexture(width, height, channels, data);
        // BGE_LOG_INFO("AssetManager", "Texture loaded and sent to GPU: " + path + " ID: " + std::to_string(textureId));
    } else {
        // BGE_LOG_ERROR("AssetManager", "Renderer service not found. Cannot create GPU texture for: " + path);
        ::stbi_image_free(data);
        return nullptr;
    }

    auto texture = std::make_shared<BGE::Texture>();
    texture->path = path;
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    texture->rendererId = textureId; // Store actual ID

    try {
        texture->lastWriteTime = std::filesystem::last_write_time(path);
    } catch (const std::filesystem::filesystem_error& e) {
        // Handle error if file not found or other issues
        // BGE_LOG_ERROR("AssetManager", "Error getting last write time for " + path + ": " + e.what());
        std::cerr << "Error getting last write time for " << path << ": " << e.what() << std::endl;
        ::stbi_image_free(data);
        // If renderer created a texture, we should ideally delete it here.
        if (renderer && textureId != 0) {
            renderer->DeleteTexture(textureId);
        }
        return nullptr;
    }

    ::stbi_image_free(data); // Data is uploaded to GPU, so free CPU copy

    m_textureCache[path] = texture;
    return texture;
}

void BGE::AssetManager::Update() {
    auto renderer = BGE::ServiceLocator::Instance().GetService<BGE::Renderer>();
    if (!renderer) {
        // BGE_LOG_ERROR("AssetManager", "Renderer service not found. Cannot update textures.");
        return;
    }

    for (auto& pair : m_textureCache) {
        const std::string& path = pair.first;
        auto& texture = pair.second;

        try {
            auto currentWriteTime = std::filesystem::last_write_time(path);
            if (currentWriteTime > texture->lastWriteTime) {
                // BGE_LOG_INFO("AssetManager", "Detected change in texture: " + path + ". Reloading...");

                int width, height, channels;
                unsigned char* data = ::stbi_load(path.c_str(), &width, &height, &channels, 0);

                if (data) {
                    renderer->UpdateTexture(texture->rendererId, width, height, channels, data);
                    texture->lastWriteTime = currentWriteTime;
                    texture->width = width;
                    texture->height = height;
                    texture->channels = channels;
                    // BGE_LOG_INFO("AssetManager", "Texture reloaded and updated on GPU: " + path);
                    ::stbi_image_free(data);
                } else {
                    // BGE_LOG_ERROR("AssetManager", "Error reloading texture: " + path + " STBI: " + stbi_failure_reason());
                     std::cerr << "Error reloading texture: " << path << std::endl;
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // BGE_LOG_ERROR("AssetManager", "Error checking/updating texture " + path + ": " + e.what());
            std::cerr << "Error checking/updating texture " << path << ": " << e.what() << std::endl;
            // Optionally, remove texture from cache or mark as invalid
        }
    }
}

} // namespace BGE