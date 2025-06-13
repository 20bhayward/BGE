#pragma once

#include <memory>
#include <string>
#include <functional>

namespace BGE {

class Application;
class Renderer;
class SimulationWorld;
class AudioSystem;
class InputManager;
class AssetManager;
class Window;
class UISystem;

struct EngineConfig {
    std::string appName = "BGE Application";
    uint32_t windowWidth = 1920;
    uint32_t windowHeight = 1080;
    bool fullscreen = false;
    bool vsync = true;
    bool enableRaytracing = true;
    bool enableMultithreading = true;
    uint32_t maxThreads = 0; // 0 = auto-detect
    std::string assetPath = "Assets/";
};

class Engine {
public:
    static Engine& Instance();
    
    bool Initialize(const EngineConfig& config);
    void Shutdown();
    
    void Run(std::unique_ptr<Application> app);
    
    // Core systems access
    Renderer* GetRenderer() const { return m_renderer.get(); }
    SimulationWorld* GetWorld() const { return m_world.get(); }
    AudioSystem* GetAudio() const { return m_audio.get(); }
    InputManager* GetInput() const { return m_input.get(); }
    AssetManager* GetAssets() const { return m_assets.get(); }
    UISystem* GetUI() const { return m_ui.get(); }
    
    // Engine state
    bool IsRunning() const { return m_running; }
    void RequestShutdown() { m_running = false; }
    
    float GetDeltaTime() const { return m_deltaTime; }
    uint64_t GetFrameCount() const { return m_frameCount; }
    
    // Events
    using ShutdownCallback = std::function<void()>;
    void RegisterShutdownCallback(ShutdownCallback callback);

private:
    Engine() = default;
    ~Engine() = default;
    
    void MainLoop();
    void Update(float deltaTime);
    void Render();
    
    bool m_initialized = false;
    bool m_running = false;
    float m_deltaTime = 0.0f;
    uint64_t m_frameCount = 0;
    
    EngineConfig m_config;
    std::unique_ptr<Application> m_application;
    
    // Core systems
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<SimulationWorld> m_world;
    std::unique_ptr<AudioSystem> m_audio;
    std::unique_ptr<InputManager> m_input;
    std::unique_ptr<AssetManager> m_assets;
    std::unique_ptr<UISystem> m_ui;
    
    std::vector<ShutdownCallback> m_shutdownCallbacks;
};

} // namespace BGE