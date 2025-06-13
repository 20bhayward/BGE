#include "../../Core/Engine.h"
#include "../../Core/Application.h"
#include "../../Core/Input/MaterialTools.h"
#include "../../Core/UI/MaterialEditorUI.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include <iostream>

using namespace BGE;

class InteractiveEditorApp : public Application {
public:
    bool Initialize() override {
        std::cout << "=== BGE Interactive Material Editor ===" << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  1-8: Select materials from palette" << std::endl;
        std::cout << "  0: Eraser" << std::endl;
        std::cout << "  Left Click: Paint" << std::endl;
        std::cout << "  Right Click: Erase" << std::endl;
        std::cout << "  P: Pause/Play simulation" << std::endl;
        std::cout << "  S: Step one frame" << std::endl;
        std::cout << "  R: Reset simulation" << std::endl;
        std::cout << "  C: Clear world" << std::endl;
        std::cout << "  [/]: Brush size" << std::endl;
        std::cout << "  B: Brush tool" << std::endl;
        std::cout << "  E: Eraser tool" << std::endl;
        std::cout << "  I: Sample tool" << std::endl;
        std::cout << "Material Palette:" << std::endl;
        std::cout << "  Sand, Water, Fire, Wood, Stone, Oil, Steam, Natural Gas," << std::endl;
        std::cout << "  Thick Gas, Smoke, Poison Gas, Ash" << std::endl;
        std::cout << "=======================================" << std::endl;
        
        // Get engine systems
        m_world = Engine::Instance().GetWorld();
        m_materials = m_world->GetMaterialSystem();
        
        // Create materials
        CreateMaterials();
        
        // Initialize material tools
        if (!m_materialTools.Initialize(m_world)) {
            std::cerr << "Failed to initialize material tools!" << std::endl;
            return false;
        }
        
        // Set up viewport (full window for now)
        m_materialTools.SetViewport(0, 0, 1280, 720);
        
        // Initialize UI
        m_editorUI.Initialize(&m_materialTools, m_world);
        
        // Start with simulation paused for editing
        m_world->Pause();
        
        // Set up initial world
        SetupInitialWorld();
        
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
        std::cout << "Mouse pressed: button=" << button << " at (" << x << ", " << y << ")" << std::endl;
        m_materialTools.OnMousePressed(button, x, y);
    }
    
    void OnMouseReleased(int button, float x, float y) override {
        std::cout << "Mouse released: button=" << button << " at (" << x << ", " << y << ")" << std::endl;
        m_materialTools.OnMouseReleased(button, x, y);
    }
    
    void OnMouseMoved(float x, float y) override {
        m_materialTools.OnMouseMoved(x, y);
    }
    
    void OnKeyPressed(int key) override {
        std::cout << "Key pressed: " << key << " ('" << static_cast<char>(key) << "')" << std::endl;
        m_materialTools.OnKeyPressed(key);
        
        // Additional controls
        switch (key) {
            case 32: // Spacebar - quick pause/play
                m_materialTools.ToggleSimulation();
                break;
            case 'C': case 'c': // Clear world
                m_world->Clear();
                std::cout << "World cleared" << std::endl;
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
        
        std::cout << "Created " << m_materials->GetMaterialCount() << " materials" << std::endl;
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
        
        std::cout << "World dimensions: " << width << "x" << height << std::endl;
        
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
        
        std::cout << "Initial world setup complete. Ready for editing!" << std::endl;
        std::cout << "Simulation is PAUSED - press P to start" << std::endl;
        std::cout << "Try clicking to paint, pressing number keys 1-8 to select materials" << std::endl;
    }
    
    SimulationWorld* m_world = nullptr;
    MaterialSystem* m_materials = nullptr;
    MaterialTools m_materialTools;
    MaterialEditorUI m_editorUI;
};

int main() {
    // Configure engine for editor use
    EngineConfig config;
    config.appName = "BGE Interactive Material Editor";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.enableRaytracing = false; // Keep it simple for editor
    
    // Initialize engine
    Engine& engine = Engine::Instance();
    if (!engine.Initialize(config)) {
        std::cerr << "Failed to initialize BGE engine!" << std::endl;
        return -1;
    }
    
    // Create and run application
    auto app = std::make_unique<InteractiveEditorApp>();
    engine.Run(std::move(app));
    
    // Cleanup
    engine.Shutdown();
    return 0;
}