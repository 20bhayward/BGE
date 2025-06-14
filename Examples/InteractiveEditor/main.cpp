#include "../../Core/Engine.h"
#include "../../Core/Application.h"
#include "../../Core/Services.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events.h"
#include "../../Core/Logger.h"
#include "../../Core/ConfigManager.h"
#include "../../Core/Entity.h"
#include "../../Core/Components.h"
#include "../../Core/Input/MaterialTools.h"
#include "../../Core/UI/MaterialEditorUI.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include "../../Core/Input/InputManager.h" // For InputManager and Keys
#include "../../Core/Input/Keyboard.h"    // For Keys::K
#include "../../Renderer/ParticleSystem.h" // For ParticleSystem type if needed, or just Services
#include "../../Renderer/Renderer.h" // For SetSimulationViewport method
#include "../../Simulation/Materials/MaterialDatabase.h" // Added for MaterialDatabase
// Core/Services.h is already included

using namespace BGE;

class InteractiveEditorApp : public Application {
public:
    bool Initialize() override {
        BGE_LOG_INFO("InteractiveEditor", "=== BGE Interactive Material Editor ===");
        BGE_LOG_INFO("InteractiveEditor", "Controls:");
        BGE_LOG_INFO("InteractiveEditor", "  1-8: Select materials from palette");
        BGE_LOG_INFO("InteractiveEditor", "  0: Eraser");
        BGE_LOG_INFO("InteractiveEditor", "  Left Click: Paint");
        BGE_LOG_INFO("InteractiveEditor", "  Right Click: Erase");
        BGE_LOG_INFO("InteractiveEditor", "  P: Pause/Play simulation");
        BGE_LOG_INFO("InteractiveEditor", "  S: Step one frame");
        BGE_LOG_INFO("InteractiveEditor", "  R: Reset simulation");
        BGE_LOG_INFO("InteractiveEditor", "  C: Clear world");
        BGE_LOG_INFO("InteractiveEditor", "  [/]: Brush size");
        BGE_LOG_INFO("InteractiveEditor", "  B: Brush tool");
        BGE_LOG_INFO("InteractiveEditor", "  E: Eraser tool");
        BGE_LOG_INFO("InteractiveEditor", "  I: Sample tool");
        BGE_LOG_INFO("InteractiveEditor", "Material Palette:");
        BGE_LOG_INFO("InteractiveEditor", "  Sand, Water, Fire, Wood, Stone, Oil, Steam, Natural Gas,");
        BGE_LOG_INFO("InteractiveEditor", "  Thick Gas, Smoke, Poison Gas, Ash");
        BGE_LOG_INFO("InteractiveEditor", "=======================================");
        
        // Subscribe to engine events
        SubscribeToEvents();
        
        // Get engine systems through service locator
        m_world = Services::GetWorld();
        if (!m_world) {
            BGE_LOG_ERROR("InteractiveEditor", "Failed to get SimulationWorld service");
            return false;
        }
        
        m_materials = m_world->GetMaterialSystem();
        
        // Load materials from file
        if (m_materials) {
            BGE::MaterialDatabase materialDB;
            if (!materialDB.LoadFromFile("Assets/Data/materials.json", *m_materials)) {
                BGE_LOG_ERROR("InteractiveEditorApp", "Failed to load materials from Assets/Data/materials.json. Trying absolute path...");
                // Try absolute path as fallback
                if (materialDB.LoadFromFile("../../../../Assets/Data/materials.json", *m_materials)) {
                    BGE_LOG_INFO("InteractiveEditorApp", "Successfully loaded materials from absolute path");
                } else {
                    BGE_LOG_ERROR("InteractiveEditorApp", "Failed to load materials from both relative and absolute paths. Falling back to CreateMaterials().");
                    // Fallback or error handling if JSON loading fails
                    CreateMaterials(); // Keep this as a fallback if JSON loading fails.
                }
            } else {
                 BGE_LOG_INFO("InteractiveEditorApp", "Successfully loaded materials from Assets/Data/materials.json");
                 // If JSON loading is successful, CreateMaterials() will be emptied or deprecated.
                 // For now, we ensure it's called if loading fails, and otherwise the loaded materials are used.
                 // The task is to empty CreateMaterials(), implying JSON loading is the primary method.
                 // So, if LoadFromFile is successful, we don't call CreateMaterials().
                 // If it fails, the original CreateMaterials() will run.
                 // The next step will be to empty CreateMaterials().
            }
        } else {
            BGE_LOG_ERROR("InteractiveEditorApp", "MaterialSystem is null. Cannot load or create materials.");
            return false;
        }

        // CreateMaterials(); // Original call location - now handled above with LoadFromFile
        
        // Initialize material tools
        if (!m_materialTools.Initialize(m_world.get())) {
            BGE_LOG_ERROR("InteractiveEditor", "Failed to initialize material tools!");
            return false;
        }
        
        // Calculate simulation rendering area (exclude UI panels)
        const int PALETTE_WIDTH = 200;  // Material palette width
        const int MENU_HEIGHT = 20;     // Main menu bar height
        
        // Get window size to calculate simulation viewport
        auto renderer = Services::GetRenderer();
        int windowWidth = 1280, windowHeight = 720; // Default fallback
        if (renderer && renderer->GetWindow()) {
            renderer->GetWindow()->GetSize(windowWidth, windowHeight);
        }
        
        // Make simulation fill most of the available space (excluding UI panels)
        int simViewportX = PALETTE_WIDTH;
        int simViewportY = MENU_HEIGHT;
        int simViewportWidth = windowWidth - PALETTE_WIDTH - 20;  // Leave some margin
        int simViewportHeight = windowHeight - MENU_HEIGHT - 20;   // Leave some margin
        
        m_materialTools.SetViewport(simViewportX, simViewportY, simViewportWidth, simViewportHeight);
        
        // Also set the renderer's viewport so OpenGL renders to the correct area
        if (renderer) {
            renderer->SetSimulationViewport(simViewportX, simViewportY, simViewportWidth, simViewportHeight);
        }
        
        BGE_LOG_INFO("InteractiveEditor", "Simulation viewport set to: (" + 
                     std::to_string(simViewportX) + "," + std::to_string(simViewportY) + 
                     ") size " + std::to_string(simViewportWidth) + "x" + std::to_string(simViewportHeight));
        
        // Initialize UI
        m_editorUI.Initialize(&m_materialTools, m_world.get());
        
        // Start with simulation paused for editing
        m_world->Pause();
        
        // Set up initial world
        SetupInitialWorld();
        
        // Create some editor entities for demonstration
        CreateEditorEntities();

        // BEGIN: Added for MovementSystem test
        auto& entityManager = BGE::EntityManager::Instance();
        auto testEntity = entityManager.CreateEntity("MovingEntity");

        if (testEntity)
        {
            testEntity->AddComponent<BGE::TransformComponent>(); // Default position is {0,0,0}
            testEntity->AddComponent<BGE::VelocityComponent>(BGE::Vector3{10.0f, 0.0f, 0.0f});

            m_testEntityId = testEntity->GetID(); // Store the ID
            // Use "InteractiveEditor" for subsystem name to match existing logs
            BGE_LOG_INFO("InteractiveEditor", "Created MovingEntity with ID: " + std::to_string(m_testEntityId));
        }
        else
        {
            BGE_LOG_ERROR("InteractiveEditor", "Failed to create MovingEntity.");
        }
        // END: Added for MovementSystem test
        
        BGE_LOG_INFO("InteractiveEditor", "Interactive Editor initialized successfully");
        return true;
    }
    
    void Shutdown() override {
        m_materialTools.Shutdown();
    }
    
    void Update(float deltaTime) override {
        // Update simulation
        m_world->Update(deltaTime);
        
        // Update material tools
        m_materialTools.Update(deltaTime);

        // BEGIN: Added for MovementSystem test
        if (m_testEntityId != BGE::INVALID_ENTITY_ID) // Check if ID is valid
        {
            auto* entity = BGE::EntityManager::Instance().GetEntity(m_testEntityId);
            if (entity)
            {
                auto* transform = entity->GetComponent<BGE::TransformComponent>();
                if (transform)
                {
                    // Ensure BGE_LOG_DEBUG takes (const char* system, const char* message)
                    std::string logMessage = "Entity Position: " + std::to_string(transform->position.x);
                    BGE_LOG_DEBUG("Test", logMessage.c_str()); // Using "Test" as the system name for this log
                }
            }
            // else
            // {
                // Entity might have been destroyed, log this once perhaps
                // BGE_LOG_WARN("InteractiveEditor", "MovingEntity not found during update.");
            // }
        }
        // END: Added for MovementSystem test
    }
    
    void Render() override {
        // Render the material editor UI
        m_editorUI.Render();
    }
    
    void OnMousePressed(int button, float x, float y) override {
        BGE_LOG_DEBUG("InteractiveEditor", "Mouse pressed: button=" + std::to_string(button) + 
                     " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        m_materialTools.OnMousePressed(button, x, y);
    }
    
    void OnMouseReleased(int button, float x, float y) override {
        BGE_LOG_DEBUG("InteractiveEditor", "Mouse released: button=" + std::to_string(button) + 
                     " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        m_materialTools.OnMouseReleased(button, x, y);
    }
    
    void OnMouseMoved(float x, float y) override {
        m_materialTools.OnMouseMoved(x, y);
    }
    
    void OnKeyPressed(int key) override {
        BGE_LOG_DEBUG("InteractiveEditor", "Key pressed: " + std::to_string(key) + 
                     " ('" + std::string(1, static_cast<char>(key)) + "')");
        m_materialTools.OnKeyPressed(key);
        
        // Additional controls
        switch (key) {
            case 32: // Spacebar - quick pause/play
                m_materialTools.ToggleSimulation();
                break;
            case 'C': case 'c': // Clear world
                m_world->Clear();
                BGE_LOG_INFO("InteractiveEditor", "World cleared");
                break;
            case Keys::K: // Or use 75 if Keys::K is not defined/found
            {
                BGE_LOG_INFO("InteractiveEditorApp", "K key pressed - creating sparks!");
                auto inputManager = Services::GetInput();
                auto particleSystem = Services::GetParticles();

                if (inputManager && particleSystem) {
                    float mouseX = 0.0f, mouseY = 0.0f;
                    inputManager->GetMousePosition(mouseX, mouseY);
                    particleSystem->CreateSparks(BGE::Vector2(mouseX, mouseY), 25);
                } else {
                    if (!inputManager) BGE_LOG_ERROR("InteractiveEditorApp", "InputManager service not found for sparks.");
                    if (!particleSystem) BGE_LOG_ERROR("InteractiveEditorApp", "ParticleSystem service not found for sparks.");
                }
                break;
            }
        }
    }

private:
    void CreateMaterials() {
        // This method is now deprecated. Materials are loaded from Assets/Data/materials.json.
        // Kept as a fallback if JSON loading fails, or can be entirely emptied.
        // For the purpose of this subtask, we will empty it in a subsequent step
        // or assume that successful JSON loading means this code is effectively bypassed.
        // If LoadFromFile fails, the original hardcoded materials will be created.
        // To fulfill "Delete the entire body":
        /*
        BGE_LOG_WARNING("InteractiveEditorApp::CreateMaterials", "This method is deprecated. Materials should be loaded from JSON.");
        // Body is deleted. If LoadFromFile failed, no materials will be created by this fallback.
        */

        // Ensuring the method is empty as per instruction for the next step.
        // If LoadFromFile fails now, no default materials will be created by this fallback.
        BGE_LOG_WARNING("InteractiveEditorApp::CreateMaterials", "CreateMaterials() called. This method is deprecated and should be empty. Materials and reactions are loaded from Assets/Data/materials.json. If loading failed, no fallback materials will be created here.");
    }
    
    // Removed SetupMaterialReactions function as reactions are now data-driven from materials.json
    
    void SetupInitialWorld() {
        // Create a simple foundation for testing
        uint32_t width = m_world->GetWidth();
        uint32_t height = m_world->GetHeight();
        
        BGE_LOG_INFO("InteractiveEditor", "World dimensions: " + std::to_string(width) + "x" + std::to_string(height));
        
        // Stone foundation at bottom
        for (uint32_t x = 0; x < width; ++x) {
            for (uint32_t y = height - 30; y < height; ++y) {
                m_world->SetMaterial(x, y, 5); // Stone material ID
            }
        }
        
        // Add some visible test materials
        // Place some sand in the middle
        for (uint32_t x = width/2 - 50; x < width/2 + 50; ++x) {
            for (uint32_t y = height/2 - 20; y < height/2; ++y) {
                m_world->SetMaterial(x, y, 1); // Sand material ID
            }
        }
        
        // Place some water
        for (uint32_t x = width/4; x < width/4 + 80; ++x) {
            for (uint32_t y = height - 60; y < height - 30; ++y) {
                m_world->SetMaterial(x, y, 2); // Water material ID
            }
        }
        
        BGE_LOG_INFO("InteractiveEditor", "Initial world setup complete. Ready for editing!");
        BGE_LOG_INFO("InteractiveEditor", "Simulation is PAUSED - press P to start");
        BGE_LOG_INFO("InteractiveEditor", "Try clicking to paint, pressing number keys 1-8 to select materials");
    }
    
    void SubscribeToEvents() {
        auto& eventBus = EventBus::Instance();
        
        // Subscribe to engine events
        eventBus.Subscribe<EngineInitializedEvent>([this](const EngineInitializedEvent& event) {
            BGE_LOG_INFO("InteractiveEditor", "Received engine initialized event: " + event.message);
        });
        
        eventBus.Subscribe<FrameStartEvent>([this](const FrameStartEvent& event) {
            // Could use this for performance monitoring
            if (event.frameCount % 60 == 0) { // Log every second at 60 FPS
                BGE_LOG_TRACE("InteractiveEditor", "Frame " + std::to_string(event.frameCount) + 
                             ", Delta: " + std::to_string(event.deltaTime));
            }
        });
        
        eventBus.Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) {
            BGE_LOG_INFO("InteractiveEditor", "Window resized to " + std::to_string(event.width) + 
                        "x" + std::to_string(event.height));
            
            // Update viewport when window is resized (exclude UI panels)
            const int PALETTE_WIDTH = 200;  // Material palette width
            const int MENU_HEIGHT = 20;     // Main menu bar height
            
            // Make simulation fill most of the available space (excluding UI panels)
            int simViewportX = PALETTE_WIDTH;
            int simViewportY = MENU_HEIGHT;
            int simViewportWidth = event.width - PALETTE_WIDTH - 20;  // Leave some margin
            int simViewportHeight = event.height - MENU_HEIGHT - 20;   // Leave some margin
            
            m_materialTools.SetViewport(simViewportX, simViewportY, simViewportWidth, simViewportHeight);
            
            // Also update the renderer's viewport
            auto renderer = Services::GetRenderer();
            if (renderer) {
                renderer->SetSimulationViewport(simViewportX, simViewportY, simViewportWidth, simViewportHeight);
            }
            
            BGE_LOG_INFO("InteractiveEditor", "Updated simulation viewport to: (" + 
                         std::to_string(simViewportX) + "," + std::to_string(simViewportY) + 
                         ") size " + std::to_string(simViewportWidth) + "x" + std::to_string(simViewportHeight));
        });
    }
    
    void CreateEditorEntities() {
        auto& entityManager = EntityManager::Instance();
        
        // Create a cursor entity for demonstration
        auto cursorEntity = entityManager.CreateEntity("EditorCursor");
        cursorEntity->AddComponent<TransformComponent>(Vector3{0, 0, 0});
        cursorEntity->AddComponent<NameComponent>("Editor Cursor");
        
        // Create some material sample entities
        auto sandSample = entityManager.CreateEntity("SandSample");
        sandSample->AddComponent<TransformComponent>(Vector3{100, 100, 0});
        sandSample->AddComponent<MaterialComponent>(1); // Sand material ID
        sandSample->AddComponent<NameComponent>("Sand Sample");
        
        auto waterSample = entityManager.CreateEntity("WaterSample");
        waterSample->AddComponent<TransformComponent>(Vector3{200, 100, 0});
        waterSample->AddComponent<MaterialComponent>(2); // Water material ID
        waterSample->AddComponent<NameComponent>("Water Sample");
        
        BGE_LOG_INFO("InteractiveEditor", "Created " + std::to_string(entityManager.GetEntityCount()) + " editor entities");
    }
    
    std::shared_ptr<SimulationWorld> m_world;
    MaterialSystem* m_materials = nullptr;
    MaterialTools m_materialTools;
    MaterialEditorUI m_editorUI;

    BGE::EntityID m_testEntityId; // Added for MovementSystem test
};

int main() {
    // Configure engine for editor use
    EngineConfig config;
    config.configFile = "config.ini";
    config.logFile = "logs/interactive_editor.log";
    
    // Initialize engine with new architecture
    Engine& engine = Engine::Instance();
    if (!engine.Initialize(config)) {
        BGE_LOG_ERROR("Main", "Failed to initialize BGE engine!");
        return -1;
    }
    
    BGE_LOG_INFO("Main", "Engine initialized, starting Interactive Material Editor");
    
    // Create and run application
    auto app = std::make_unique<InteractiveEditorApp>();
    engine.Run(std::move(app));
    
    BGE_LOG_INFO("Main", "Application finished, shutting down engine");
    
    // Cleanup
    engine.Shutdown();
    return 0;
}