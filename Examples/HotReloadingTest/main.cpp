#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/ServiceLocator.h"
#include "AssetPipeline/AssetManager.h"
#include "Renderer/Texture.h" // To use std::shared_ptr<Texture>
#include "Core/Logger.h"      // For logging

class HotReloadTestApp : public BGE::Application {
public:
    std::shared_ptr<BGE::Texture> m_testTexture;

    bool Initialize() override {
        BGE_LOG_INFO("HotReloadTestApp", "Initializing Application...");

        auto assetManager = BGE::ServiceLocator::Instance().GetService<BGE::AssetManager>();
        if (!assetManager) {
            BGE_LOG_ERROR("HotReloadTestApp", "Failed to get AssetManager service.");
            return false;
        }

        // Assuming Assets/Images/test_sprite.png is relative to the execution path
        // or AssetManager handles path resolution. For tests, often execution is from project root.
        m_testTexture = assetManager->LoadTexture("Assets/Images/test_sprite.png");

        if (!m_testTexture) {
            BGE_LOG_ERROR("HotReloadTestApp", "Failed to load texture: Assets/Images/test_sprite.png");
            // This might be critical depending on the test. For now, we'll allow it to proceed
            // to test hot-reloading if the file is created later.
        } else {
            BGE_LOG_INFO("HotReloadTestApp", "Successfully loaded texture: Assets/Images/test_sprite.png");
            BGE_LOG_INFO("HotReloadTestApp", "Texture ID: " + std::to_string(m_testTexture->rendererId) +
                                         ", Width: " + std::to_string(m_testTexture->width) +
                                         ", Height: " + std::to_string(m_testTexture->height));
        }

        BGE_LOG_INFO("HotReloadTestApp", "Application Initialized. Waiting for texture changes...");
        BGE_LOG_INFO("HotReloadTestApp", "Modify 'Assets/Images/test_sprite.png' and save to test hot-reloading.");

        return true;
    }

    void Update(float deltaTime) override {
        // Application-specific update logic (if any)
        // For this test, most work is done by AssetManager::Update() called by the Engine
    }

    void Render() override {
        // Application-specific rendering logic
        // In a real app, you'd draw the m_testTexture here.
        // For now, we rely on logs from AssetManager to see updates.
        if (m_testTexture) {
            // Simulate using the texture (e.g., log its current state if it changed)
        }
    }

    void Shutdown() override {
        BGE_LOG_INFO("HotReloadTestApp", "Shutting down Application...");
        m_testTexture.reset();
    }
};

int main(int argc, char* argv[]) {
    BGE::Engine& engine = BGE::Engine::Instance();

    BGE::EngineConfig config;
    // Potentially set config.configFile if specific settings are needed for this example

    if (!engine.Initialize(config)) {
        return -1;
    }

    auto app = std::make_unique<HotReloadTestApp>();
    engine.Run(std::move(app));

    engine.Shutdown();
    return 0;
}
