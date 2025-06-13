#include "../../Core/Engine.h"
#include "../../Core/Application.h"
#include "../../Simulation/SimulationWorld.h"
#include "../../Simulation/Materials/MaterialSystem.h"

using namespace BGE;

class SandboxApp : public Application {
public:
    bool Initialize() override {
        // Get engine systems
        m_world = Engine::Instance().GetWorld();
        m_materials = m_world->GetMaterialSystem();
        
        // Create basic materials
        CreateBasicMaterials();
        
        // Set up initial world
        SetupWorld();
        
        return true;
    }
    
    void Shutdown() override {
        // Cleanup if needed
    }
    
    void Update(float deltaTime) override {
        // Update simulation
        m_world->Update(deltaTime);
        
        // Handle user input for material placement
        HandleInput();
    }
    
    void Render() override {
        // Rendering is handled automatically by the engine
        // We can add custom rendering here if needed
    }
    
    void OnMousePressed(int button, float x, float y) override {
        if (button == 0) { // Left mouse button
            // Place selected material at mouse position
            int worldX = static_cast<int>(x);
            int worldY = static_cast<int>(y);
            MaterialID materialToPlace = (m_selectedMaterial != MATERIAL_EMPTY) ? m_selectedMaterial : m_sandMaterial;
            PlaceMaterial(worldX, worldY, materialToPlace, 8);
        }
        else if (button == 1) { // Right mouse button
            // Place water at mouse position (quick access)
            int worldX = static_cast<int>(x);
            int worldY = static_cast<int>(y);
            PlaceMaterial(worldX, worldY, m_waterMaterial, 8);
        }
    }
    
    void OnKeyPressed(int key) override {
        switch (key) {
            case 32: // Spacebar - reset world
                m_world->Clear();
                SetupWorld();
                break;
            case 49: // '1' key - select sand
                m_selectedMaterial = m_sandMaterial;
                break;
            case 50: // '2' key - select water
                m_selectedMaterial = m_waterMaterial;
                break;
            case 51: // '3' key - select fire
                m_selectedMaterial = m_fireMaterial;
                break;
            case 52: // '4' key - select wood
                m_selectedMaterial = m_woodMaterial;
                break;
            case 53: // '5' key - select stone
                m_selectedMaterial = m_stoneMaterial;
                break;
            case 54: // '6' key - select oil
                m_selectedMaterial = m_oilMaterial;
                break;
            case 55: // '7' key - select steam
                m_selectedMaterial = m_steamMaterial;
                break;
        }
    }

private:
    void CreateBasicMaterials() {
        // Create sand material
        m_sandMaterial = m_materials->CreateMaterialBuilder("Sand")
            .SetColor(194, 178, 128)
            .SetBehavior(MaterialBehavior::Powder)
            .SetDensity(1.5f)
            .GetID();
        
        // Create water material
        m_waterMaterial = m_materials->CreateMaterialBuilder("Water")
            .SetColor(64, 164, 223, 180)
            .SetBehavior(MaterialBehavior::Liquid)
            .SetDensity(1.0f)
            .GetID();
        
        // Create fire material
        m_fireMaterial = m_materials->CreateMaterialBuilder("Fire")
            .SetColor(255, 100, 0)
            .SetBehavior(MaterialBehavior::Fire)
            .SetEmission(2.0f)
            .SetDensity(0.1f)
            .GetID();
        
        // Create wood material
        m_woodMaterial = m_materials->CreateMaterialBuilder("Wood")
            .SetColor(139, 69, 19)
            .SetBehavior(MaterialBehavior::Static)
            .SetDensity(0.8f)
            .GetID();
        
        // Create stone material
        m_stoneMaterial = m_materials->CreateMaterialBuilder("Stone")
            .SetColor(128, 128, 128)
            .SetBehavior(MaterialBehavior::Static)
            .SetDensity(2.5f)
            .GetID();
        
        // Create oil material
        m_oilMaterial = m_materials->CreateMaterialBuilder("Oil")
            .SetColor(40, 40, 20, 200)
            .SetBehavior(MaterialBehavior::Liquid)
            .SetDensity(0.9f)
            .GetID();
        
        // Create steam material (more visible)
        m_steamMaterial = m_materials->CreateMaterialBuilder("Steam")
            .SetColor(255, 255, 255, 180)
            .SetBehavior(MaterialBehavior::Gas)
            .SetDensity(0.1f)
            .GetID();
        
        // Create ash material
        m_ashMaterial = m_materials->CreateMaterialBuilder("Ash")
            .SetColor(64, 64, 64)
            .SetBehavior(MaterialBehavior::Powder)
            .SetDensity(0.6f)
            .GetID();
        
        // Add reaction: Fire + Wood -> Fire + Ash
        MaterialReaction burnWood;
        burnWood.reactant = m_woodMaterial;
        burnWood.product1 = m_fireMaterial;
        burnWood.product2 = m_ashMaterial;
        burnWood.probability = 0.1f;
        burnWood.requiresHeat = true;
        burnWood.minTemperature = 300.0f;
        
        m_materials->GetMaterial(m_fireMaterial).AddReaction(burnWood);
        
        // Add reaction: Water + Fire -> Steam (extinguish fire)
        MaterialReaction extinguishFire;
        extinguishFire.reactant = m_fireMaterial;
        extinguishFire.product1 = m_steamMaterial;
        extinguishFire.product2 = MATERIAL_EMPTY;
        extinguishFire.probability = 0.5f;
        extinguishFire.requiresHeat = false;
        
        m_materials->GetMaterial(m_waterMaterial).AddReaction(extinguishFire);
        
        // Add reaction: Water heated -> Steam
        MaterialReaction boilWater;
        boilWater.reactant = MATERIAL_EMPTY; // Self-reaction based on temperature
        boilWater.product1 = m_steamMaterial;
        boilWater.product2 = MATERIAL_EMPTY;
        boilWater.probability = 0.1f;
        boilWater.requiresHeat = true;
        boilWater.minTemperature = 100.0f;
        
        m_materials->GetMaterial(m_waterMaterial).AddReaction(boilWater);
    }
    
    void SetupWorld() {
        // Create a comprehensive physics demonstration
        uint32_t width = m_world->GetWidth();
        uint32_t height = m_world->GetHeight();
        
        // Create stone foundation at bottom
        for (uint32_t x = 0; x < width; ++x) {
            for (uint32_t y = height - 50; y < height; ++y) {
                m_world->SetMaterial(x, y, m_stoneMaterial);
            }
        }
        
        // Create wooden structures
        // Left wooden tower
        for (uint32_t x = width/6; x < width/6 + 30; ++x) {
            for (uint32_t y = height - 150; y < height - 50; ++y) {
                if (x == width/6 || x == width/6 + 29 || y == height - 150) {
                    m_world->SetMaterial(x, y, m_woodMaterial);
                }
            }
        }
        
        // Right wooden tower
        for (uint32_t x = 5*width/6 - 30; x < 5*width/6; ++x) {
            for (uint32_t y = height - 150; y < height - 50; ++y) {
                if (x == 5*width/6 - 30 || x == 5*width/6 - 1 || y == height - 150) {
                    m_world->SetMaterial(x, y, m_woodMaterial);
                }
            }
        }
        
        // Add sand falling from multiple points
        for (uint32_t x = width/4; x < 3*width/4; x += 15) {
            for (uint32_t y = 50; y < 120; ++y) {
                if ((x + y) % 2 == 0) {
                    m_world->SetMaterial(x, y, m_sandMaterial);
                }
            }
        }
        
        // Add water pools (sitting on top of stone foundation)
        for (uint32_t x = width/3; x < 2*width/3; ++x) {
            for (uint32_t y = height - 70; y < height - 50; ++y) {
                m_world->SetMaterial(x, y, m_waterMaterial);
            }
        }
        
        // Add some oil on top of water (density separation demo)
        for (uint32_t x = width/3 + 20; x < 2*width/3 - 20; ++x) {
            for (uint32_t y = height - 65; y < height - 55; ++y) {
                if ((x + y) % 3 == 0) {
                    m_world->SetMaterial(x, y, m_oilMaterial);
                }
            }
        }
        
        // Add fire sources next to wooden structures for combustion demo
        m_world->SetMaterial(width/6 - 2, height - 155, m_fireMaterial);
        m_world->SetMaterial(5*width/6 + 2, height - 155, m_fireMaterial);
        
        // Set high temperature on fire sources
        m_world->SetTemperature(width/6 - 2, height - 155, 800.0f);
        m_world->SetTemperature(5*width/6 + 2, height - 155, 800.0f);
        
        // Add some initial steam at top for gas behavior demo
        for (uint32_t x = width/2 - 10; x < width/2 + 10; ++x) {
            for (uint32_t y = 20; y < 40; ++y) {
                if ((x + y) % 4 == 0) {
                    m_world->SetMaterial(x, y, m_steamMaterial);
                }
            }
        }
    }
    
    void HandleInput() {
        // Additional input handling can go here
    }
    
    void PlaceMaterial(int x, int y, MaterialID material, int radius) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                if (dx * dx + dy * dy <= radius * radius) {
                    int px = x + dx;
                    int py = y + dy;
                    if (m_world->IsValidPosition(px, py)) {
                        m_world->SetMaterial(px, py, material);
                    }
                }
            }
        }
    }
    
    SimulationWorld* m_world = nullptr;
    MaterialSystem* m_materials = nullptr;
    
    MaterialID m_sandMaterial = MATERIAL_EMPTY;
    MaterialID m_waterMaterial = MATERIAL_EMPTY;
    MaterialID m_fireMaterial = MATERIAL_EMPTY;
    MaterialID m_woodMaterial = MATERIAL_EMPTY;
    MaterialID m_stoneMaterial = MATERIAL_EMPTY;
    MaterialID m_oilMaterial = MATERIAL_EMPTY;
    MaterialID m_steamMaterial = MATERIAL_EMPTY;
    MaterialID m_ashMaterial = MATERIAL_EMPTY;
    MaterialID m_selectedMaterial = MATERIAL_EMPTY;
};

int main() {
    // Configure engine
    EngineConfig config;
    config.appName = "BGE Sandbox Demo";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.enableRaytracing = true;
    
    // Initialize engine
    Engine& engine = Engine::Instance();
    if (!engine.Initialize(config)) {
        return -1;
    }
    
    // Create and run application
    auto app = std::make_unique<SandboxApp>();
    engine.Run(std::move(app));
    
    // Cleanup
    engine.Shutdown();
    return 0;
}