#pragma once

#include <memory>
#include <string>
#include <functional>

#include "SystemManager.h" // Required for std::unique_ptr<SystemManager>

namespace BGE {

class Application;
class Window;

struct EngineConfig {
    std::string configFile = "config.ini";
    std::string logFile = "logs/bge.log";
};

class Engine {
public:
    static Engine& Instance();
    
    bool Initialize(const EngineConfig& config = {});
    void Shutdown();
    
    void Run(std::unique_ptr<Application> app);
    
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
    
    bool InitializeServices();
    void RegisterCoreServices();
    void MainLoop();
    void Update(float deltaTime);
    void Render();
    
    bool m_initialized = false;
    bool m_running = false;
    float m_deltaTime = 0.0f;
    uint64_t m_frameCount = 0;
    
    EngineConfig m_config;
    std::unique_ptr<Application> m_application;
    std::unique_ptr<Window> m_window;
    
    std::vector<ShutdownCallback> m_shutdownCallbacks;

    std::unique_ptr<SystemManager> m_systemManager;
};

} // namespace BGE