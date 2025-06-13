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
        
        // Create materials
        CreateMaterials();
        
        // Initialize material tools
        if (!m_materialTools.Initialize(m_world.get())) {
            BGE_LOG_ERROR("InteractiveEditor", "Failed to initialize material tools!");
            return false;
        }
        
        // Get viewport settings from config
        auto& config = ConfigManager::Instance();
        int windowWidth = config.GetInt("window.width", 1280);
        int windowHeight = config.GetInt("window.height", 720);
        m_materialTools.SetViewport(0, 0, windowWidth, windowHeight);
        
        // Initialize UI
        m_editorUI.Initialize(&m_materialTools, m_world.get());
        
        // Start with simulation paused for editing
        m_world->Pause();
        
        // Set up initial world
        SetupInitialWorld();
        
        // Create some editor entities for demonstration
        CreateEditorEntities();
        
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
        }
    }

private:
    void CreateMaterials() {
        // Create sand material
        m_materials->CreateMaterialBuilder("Sand")
            .SetColor(194, 178, 128)
            .SetBehavior(MaterialBehavior::Powder)
            .SetDensity(1.5f)
            .GetID();
        
        // Create water material
        MaterialID water = m_materials->CreateMaterialBuilder("Water")
            .SetColor(64, 164, 223, 180)
            .SetBehavior(MaterialBehavior::Liquid)
            .SetDensity(1.0f)
            .GetID();
        
        // Create fire material
        MaterialID fire = m_materials->CreateMaterialBuilder("Fire")
            .SetColor(255, 100, 0)
            .SetBehavior(MaterialBehavior::Fire)
            .SetEmission(2.0f)
            .SetDensity(0.1f)
            .GetID();
        
        // Create wood material
        MaterialID wood = m_materials->CreateMaterialBuilder("Wood")
            .SetColor(139, 69, 19)
            .SetBehavior(MaterialBehavior::Static)
            .SetDensity(0.8f)
            .GetID();
        
        // Create stone material
        m_materials->CreateMaterialBuilder("Stone")
            .SetColor(128, 128, 128)
            .SetBehavior(MaterialBehavior::Static)
            .SetDensity(2.5f)
            .GetID();
        
        // Create oil material
        m_materials->CreateMaterialBuilder("Oil")
            .SetColor(40, 40, 20, 200)
            .SetBehavior(MaterialBehavior::Liquid)
            .SetDensity(0.9f)
            .GetID();
        
        // Create steam material
        MaterialID steam = m_materials->CreateMaterialBuilder("Steam")
            .SetColor(255, 255, 255, 180)
            .SetBehavior(MaterialBehavior::Gas)
            .SetDensity(0.1f)
            .GetID();
        
        // Create natural gas (light, fast-rising)
        MaterialID naturalGas = m_materials->CreateMaterialBuilder("NaturalGas")
            .SetColor(200, 255, 200, 120)  // Light green, semi-transparent
            .SetBehavior(MaterialBehavior::Gas)
            .SetDensity(0.05f)  // Very light - rises quickly
            .GetID();
        
        // Create thick gas (heavier, slower)
        MaterialID thickGas = m_materials->CreateMaterialBuilder("ThickGas")
            .SetColor(150, 150, 255, 160)  // Light blue, more opaque
            .SetBehavior(MaterialBehavior::Gas)
            .SetDensity(0.3f)   // Heavier - rises slower, spreads more
            .GetID();
        
        // Create smoke (dark, disperses over time)
        MaterialID smoke = m_materials->CreateMaterialBuilder("Smoke")
            .SetColor(80, 80, 80, 140)     // Dark gray, semi-transparent
            .SetBehavior(MaterialBehavior::Gas)
            .SetDensity(0.08f)  // Light but not as light as natural gas
            .GetID();
        
        // Create poison gas (dangerous, visible)
        MaterialID poisonGas = m_materials->CreateMaterialBuilder("PoisonGas")
            .SetColor(100, 255, 100, 180)  // Sickly green, more visible
            .SetBehavior(MaterialBehavior::Gas)
            .SetDensity(0.15f)  // Medium density - spreads at moderate speed
            .GetID();
        
        // Create ash material
        MaterialID ash = m_materials->CreateMaterialBuilder("Ash")
            .SetColor(64, 64, 64)
            .SetBehavior(MaterialBehavior::Powder)
            .SetDensity(0.6f)
            .GetID();
        
        // Add material reactions
        SetupMaterialReactions(fire, wood, ash, water, steam, naturalGas, thickGas, smoke, poisonGas);
        
        BGE_LOG_INFO("InteractiveEditor", "Created " + std::to_string(m_materials->GetMaterialCount()) + " materials");
    }
    
    void SetupMaterialReactions(MaterialID fire, MaterialID wood, MaterialID ash, MaterialID water, MaterialID steam, 
                               MaterialID naturalGas, MaterialID thickGas, MaterialID smoke, MaterialID poisonGas) {
        // Fire + Wood -> Fire + Ash
        MaterialReaction burnWood;
        burnWood.reactant = wood;
        burnWood.product1 = fire;
        burnWood.product2 = ash;
        burnWood.probability = 0.05f;
        burnWood.requiresHeat = true;
        burnWood.minTemperature = 300.0f;
        
        m_materials->GetMaterial(fire).AddReaction(burnWood);
        
        // Water + Fire -> Steam (conserve mass - water becomes steam, fire is extinguished)
        MaterialReaction extinguishFire;
        extinguishFire.reactant = fire;
        extinguishFire.product1 = steam;  // Water becomes steam
        extinguishFire.product2 = MATERIAL_EMPTY;  // Fire is extinguished (disappears)
        extinguishFire.probability = 0.05f;  // Lower probability for more realistic behavior
        extinguishFire.requiresHeat = false;
        
        m_materials->GetMaterial(water).AddReaction(extinguishFire);
        
        // Natural Gas + Fire -> Fire + Smoke (combustion)
        MaterialReaction burnGas;
        burnGas.reactant = fire;
        burnGas.product1 = fire;  // Fire spreads
        burnGas.product2 = smoke; // Produces smoke
        burnGas.probability = 0.2f;
        burnGas.requiresHeat = false;
        
        m_materials->GetMaterial(naturalGas).AddReaction(burnGas);
        
        // Fire + Wood -> Fire + Smoke (realistic combustion)
        MaterialReaction woodSmoke;
        woodSmoke.reactant = wood;
        woodSmoke.product1 = fire;
        woodSmoke.product2 = smoke; // Wood burning produces smoke
        woodSmoke.probability = 0.03f;
        woodSmoke.requiresHeat = true;
        woodSmoke.minTemperature = 350.0f;
        
        m_materials->GetMaterial(fire).AddReaction(woodSmoke);
        
        // Water + Poison Gas -> Water + Thick Gas (dilution)
        MaterialReaction dilutePoison;
        dilutePoison.reactant = poisonGas;
        dilutePoison.product1 = water;
        dilutePoison.product2 = thickGas; // Poison becomes less dangerous
        dilutePoison.probability = 0.1f;
        dilutePoison.requiresHeat = false;
        
        m_materials->GetMaterial(water).AddReaction(dilutePoison);
    }
    
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
            // Update viewport when window is resized
            m_materialTools.SetViewport(0, 0, event.width, event.height);
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