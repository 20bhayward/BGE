#include "Engine.h"
#include "Application.h"
#include "Platform/Window.h"
#include "Threading/ThreadPool.h"
#include "Time/Timer.h"
#include "Input/InputManager.h"
#include "Memory/MemoryPool.h"
#include "../Simulation/SimulationWorld.h"
#include "../Renderer/Renderer.h"
#include "../Audio/AudioSystem.h"
#include "../AssetPipeline/AssetManager.h"
#include "UI/UISystem.h"

#include <chrono>
#include <iostream>

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
    
    std::cout << "Initializing BGE Engine..." << std::endl;
    std::cout << "App: " << config.appName << std::endl;
    std::cout << "Resolution: " << config.windowWidth << "x" << config.windowHeight << std::endl;
    
    try {
        // Initialize core systems
        std::cout << "Creating window..." << std::endl;
        m_window = std::make_unique<Window>();
        WindowConfig windowConfig;
        windowConfig.width = config.windowWidth;
        windowConfig.height = config.windowHeight;
        windowConfig.title = config.appName;
        if (!m_window->Initialize(windowConfig)) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }
        
        std::cout << "Initializing renderer..." << std::endl;
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->Initialize(m_window.get())) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }
        
        std::cout << "Creating simulation world..." << std::endl;
        m_world = std::make_unique<SimulationWorld>(config.windowWidth, config.windowHeight);
        
        std::cout << "Initializing input manager..." << std::endl;
        m_input = std::make_unique<InputManager>();
        m_input->Initialize();
        
        // Connect input manager to window
        m_window->SetInputManager(m_input.get());
        
        std::cout << "Initializing audio system..." << std::endl;
        m_audio = std::make_unique<AudioSystem>();
        m_audio->Initialize();
        
        std::cout << "Initializing asset manager..." << std::endl;
        m_assets = std::make_unique<AssetManager>();
        m_assets->Initialize();
        
        std::cout << "Initializing UI system..." << std::endl;
        m_ui = std::make_unique<UISystem>();
        if (!m_ui->Initialize(m_window.get())) {
            std::cerr << "Failed to initialize UI system" << std::endl;
            return false;
        }
        
        m_initialized = true;
        std::cout << "BGE Engine initialized successfully!" << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Engine initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void Engine::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down BGE Engine..." << std::endl;
    
    // Call shutdown callbacks
    for (const auto& callback : m_shutdownCallbacks) {
        callback();
    }
    
    m_running = false;
    
    // Shutdown systems in reverse order
    if (m_application) {
        m_application->Shutdown();
        m_application.reset();
    }
    
    if (m_ui) {
        m_ui->Shutdown();
        m_ui.reset();
    }
    
    m_assets.reset();
    m_audio.reset();
    m_input.reset();
    m_world.reset();
    m_renderer.reset();
    m_window.reset();
    
    m_initialized = false;
    std::cout << "BGE Engine shutdown complete." << std::endl;
}

void Engine::Run(std::unique_ptr<Application> app) {
    if (!m_initialized) {
        std::cerr << "Engine not initialized!" << std::endl;
        return;
    }
    
    m_application = std::move(app);
    
    // Connect application to input manager
    m_input->SetApplication(m_application.get());
    
    if (!m_application->Initialize()) {
        std::cerr << "Application initialization failed!" << std::endl;
        return;
    }
    
    std::cout << "Starting main loop..." << std::endl;
    m_running = true;
    MainLoop();
}

void Engine::MainLoop() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    const float targetFPS = 60.0f;
    const float targetFrameTime = 1.0f / targetFPS;
    
    while (m_running && !m_window->ShouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        m_deltaTime = std::min(deltaTime, 0.033f); // Cap at ~30 FPS minimum
        
        // Poll events
        m_window->PollEvents();
        m_input->Update();
        
        // Update application
        Update(m_deltaTime);
        
        // Render
        Render();
        
        // Present
        m_window->SwapBuffers();
        
        ++m_frameCount;
        
        // Simple frame rate limiting
        auto frameTime = std::chrono::high_resolution_clock::now() - currentTime;
        auto frameTimeFloat = std::chrono::duration<float>(frameTime).count();
        if (frameTimeFloat < targetFrameTime) {
            auto sleepTime = targetFrameTime - frameTimeFloat;
            std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
    }
    
    std::cout << "Main loop ended after " << m_frameCount << " frames." << std::endl;
}

void Engine::Update(float deltaTime) {
    if (m_application) {
        m_application->Update(deltaTime);
    }
    
    if (m_world) {
        m_world->Update(deltaTime);
    }
    
    if (m_audio) {
        m_audio->Update(deltaTime);
    }
}

void Engine::Render() {
    if (m_renderer) {
        m_renderer->BeginFrame();
        
        // Render simulation world
        if (m_world) {
            m_renderer->RenderWorld(m_world.get());
        }
        
        // Begin UI frame
        if (m_ui) {
            m_ui->BeginFrame();
        }
        
        // Render application (including UI)
        if (m_application) {
            m_application->Render();
        }
        
        // End UI frame and render UI
        if (m_ui) {
            m_ui->EndFrame();
        }
        
        m_renderer->EndFrame();
    }
}

void Engine::RegisterShutdownCallback(ShutdownCallback callback) {
    m_shutdownCallbacks.push_back(callback);
}

} // namespace BGE