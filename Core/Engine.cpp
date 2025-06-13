#include "Engine.h"
#include "Application.h"
#include "Platform/Window.h"
#include "ServiceLocator.h"
#include "EventBus.h"
#include "Events.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "Entity.h"
#include "Input/InputManager.h"
#include "../Simulation/SimulationWorld.h"
#include "../Renderer/Renderer.h"
#include "../Audio/AudioSystem.h"
#include "../AssetPipeline/AssetManager.h"
#include "UI/UISystem.h"

#include <chrono>
#include <filesystem>
#include <thread>

namespace BGE {

Engine& Engine::Instance() {
    static Engine instance;
    return instance;
}

bool Engine::Initialize(const EngineConfig& config) {
    if (m_initialized) {
        return true;
    }
    
    m_config = config;
    
    // Initialize core foundation systems first
    auto& logger = Logger::Instance();
    auto& configManager = ConfigManager::Instance();
    
    // Create logs directory if it doesn't exist
    if (!config.logFile.empty()) {
        std::filesystem::path logPath(config.logFile);
        std::filesystem::create_directories(logPath.parent_path());
    }
    
    // Initialize logger
    logger.Initialize(config.logFile);
    BGE_LOG_INFO("Engine", "Initializing BGE Engine...");
    
    // Load configuration
    if (!config.configFile.empty() && std::filesystem::exists(config.configFile)) {
        if (configManager.LoadFromFile(config.configFile)) {
            BGE_LOG_INFO("Engine", "Configuration loaded from: " + config.configFile);
        } else {
            BGE_LOG_WARNING("Engine", "Failed to load configuration from: " + config.configFile);
        }
    }
    
    // Initialize services
    if (!InitializeServices()) {
        BGE_LOG_ERROR("Engine", "Failed to initialize core services");
        return false;
    }
    
    m_initialized = true;
    
    // Fire engine initialized event
    EventBus::Instance().Publish(EngineInitializedEvent{true, "Engine initialized successfully"});
    BGE_LOG_INFO("Engine", "BGE Engine initialized successfully!");
    
    return true;
}

bool Engine::InitializeServices() {
    auto& configManager = ConfigManager::Instance();
    
    BGE_LOG_INFO("Engine", "Initializing core services...");
    
    try {
        // Create window
        BGE_LOG_INFO("Engine", "Creating window...");
        m_window = std::make_unique<Window>();
        WindowConfig windowConfig;
        windowConfig.width = configManager.GetInt("window.width", 1920);
        windowConfig.height = configManager.GetInt("window.height", 1080);
        windowConfig.title = configManager.GetString("window.title", "BGE Application");
        
        if (!m_window->Initialize(windowConfig)) {
            BGE_LOG_ERROR("Engine", "Failed to create window");
            return false;
        }
        
        // Register core services
        RegisterCoreServices();
        
        return true;
    }
    catch (const std::exception& e) {
        BGE_LOG_ERROR("Engine", "Service initialization failed: " + std::string(e.what()));
        return false;
    }
}

void Engine::RegisterCoreServices() {
    auto& serviceLocator = ServiceLocator::Instance();
    auto& configManager = ConfigManager::Instance();
    
    BGE_LOG_INFO("Engine", "Registering core services...");
    
    // Initialize and register renderer
    auto renderer = std::make_shared<Renderer>();
    if (renderer->Initialize(m_window.get())) {
        serviceLocator.RegisterService<Renderer>(renderer);
        BGE_LOG_INFO("Engine", "Renderer service registered");
    } else {
        BGE_LOG_ERROR("Engine", "Failed to initialize renderer");
    }
    
    // Initialize and register simulation world
    int worldWidth = configManager.GetInt("simulation.world_width", 512);
    int worldHeight = configManager.GetInt("simulation.world_height", 512);
    auto world = std::make_shared<SimulationWorld>(worldWidth, worldHeight);
    serviceLocator.RegisterService<SimulationWorld>(world);
    BGE_LOG_INFO("Engine", "SimulationWorld service registered");
    
    // Initialize and register input manager
    auto input = std::make_shared<InputManager>();
    if (input->Initialize()) {
        m_window->SetInputManager(input.get());
        serviceLocator.RegisterService<InputManager>(input);
        BGE_LOG_INFO("Engine", "InputManager service registered");
    }
    
    // Initialize and register audio system
    auto audio = std::make_shared<AudioSystem>();
    if (audio->Initialize()) {
        serviceLocator.RegisterService<AudioSystem>(audio);
        BGE_LOG_INFO("Engine", "AudioSystem service registered");
    }
    
    // Initialize and register asset manager
    auto assets = std::make_shared<AssetManager>();
    if (assets->Initialize()) {
        serviceLocator.RegisterService<AssetManager>(assets);
        BGE_LOG_INFO("Engine", "AssetManager service registered");
    }
    
    // Initialize and register UI system
    auto ui = std::make_shared<UISystem>();
    if (ui->Initialize(m_window.get())) {
        serviceLocator.RegisterService<UISystem>(ui);
        BGE_LOG_INFO("Engine", "UISystem service registered");
    }
}

void Engine::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    BGE_LOG_INFO("Engine", "Shutting down BGE Engine...");
    
    // Fire engine shutting down event  
    EventBus::Instance().Publish(EngineShuttingDownEvent{"Normal shutdown"});
    
    // Call shutdown callbacks
    for (const auto& callback : m_shutdownCallbacks) {
        callback();
    }
    
    m_running = false;
    
    // Shutdown application
    if (m_application) {
        m_application->Shutdown();
        m_application.reset();
    }
    
    // Clear services
    auto& serviceLocator = ServiceLocator::Instance();
    if (auto ui = serviceLocator.GetService<UISystem>()) {
        ui->Shutdown();
    }
    serviceLocator.Clear();
    
    // Clear entity manager
    EntityManager::Instance().Clear();
    
    // Clear event bus
    EventBus::Instance().Clear();
    
    // Shutdown window
    m_window.reset();
    
    m_initialized = false;
    BGE_LOG_INFO("Engine", "BGE Engine shutdown complete");
    
    // Shutdown logger last
    Logger::Instance().Shutdown();
}

void Engine::Run(std::unique_ptr<Application> app) {
    if (!m_initialized) {
        BGE_LOG_ERROR("Engine", "Engine not initialized!");
        return;
    }
    
    m_application = std::move(app);
    
    // Connect application to input manager
    auto input = ServiceLocator::Instance().GetService<InputManager>();
    if (input) {
        input->SetApplication(m_application.get());
    }
    
    if (!m_application->Initialize()) {
        BGE_LOG_ERROR("Engine", "Application initialization failed!");
        return;
    }
    
    BGE_LOG_INFO("Engine", "Starting main loop...");
    m_running = true;
    MainLoop();
}

void Engine::MainLoop() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    const float targetFPS = ConfigManager::Instance().GetFloat("simulation.update_frequency", 60.0f);
    const float targetFrameTime = 1.0f / targetFPS;
    
    auto input = ServiceLocator::Instance().GetService<InputManager>();
    
    while (m_running && !m_window->ShouldClose()) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(frameStartTime - lastTime).count();
        lastTime = frameStartTime;
        
        m_deltaTime = std::min(deltaTime, 0.033f); // Cap at ~30 FPS minimum
        
        // Fire frame start event
        EventBus::Instance().Publish(FrameStartEvent{m_deltaTime, m_frameCount});
        
        // Poll events
        m_window->PollEvents();
        if (input) {
            input->Update();
        }
        
        // Update application
        Update(m_deltaTime);
        
        // Render
        Render();
        
        // Present
        m_window->SwapBuffers();
        
        ++m_frameCount;
        
        // Calculate frame time
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<float>(frameEndTime - frameStartTime).count();
        
        // Fire frame end event
        EventBus::Instance().Publish(FrameEndEvent{m_deltaTime, m_frameCount, frameTime});
        
        // Simple frame rate limiting
        if (frameTime < targetFrameTime) {
            auto sleepTime = targetFrameTime - frameTime;
            std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
    }
    
    BGE_LOG_INFO("Engine", "Main loop ended after " + std::to_string(m_frameCount) + " frames");
}

void Engine::Update(float deltaTime) {
    auto& serviceLocator = ServiceLocator::Instance();
    
    // Update application
    if (m_application) {
        m_application->Update(deltaTime);
    }
    
    // Update simulation world
    if (auto world = serviceLocator.GetService<SimulationWorld>()) {
        world->Update(deltaTime);
    }
    
    // Update audio system
    if (auto audio = serviceLocator.GetService<AudioSystem>()) {
        audio->Update(deltaTime);
    }
}

void Engine::Render() {
    auto& serviceLocator = ServiceLocator::Instance();
    auto renderer = serviceLocator.GetService<Renderer>();
    auto world = serviceLocator.GetService<SimulationWorld>();
    auto ui = serviceLocator.GetService<UISystem>();
    
    if (renderer) {
        renderer->BeginFrame();
        
        // Render simulation world
        if (world) {
            renderer->RenderWorld(world.get());
        }
        
        // Begin UI frame
        if (ui) {
            ui->BeginFrame();
        }
        
        // Render application (including UI)
        if (m_application) {
            m_application->Render();
        }
        
        // End UI frame and render UI
        if (ui) {
            ui->EndFrame();
        }
        
        renderer->EndFrame();
    }
}

void Engine::RegisterShutdownCallback(ShutdownCallback callback) {
    m_shutdownCallbacks.push_back(callback);
}

} // namespace BGE