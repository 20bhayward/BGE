#include "Core/Engine.h"
#include "Core/Application.h"
#include "Simulation/SimulationWorld.h"
#include "Simulation/Materials/MaterialSystem.h"
#include "Renderer/Lighting/LightingSystem.h"

using namespace BGE;

class LightingDemoApp : public Application {
public:
    bool Initialize() override {
        m_world = Engine::Instance().GetWorld();
        m_materials = m_world->GetMaterialSystem();
        m_lighting = Engine::Instance().GetRenderer()->GetLightingSystem();
        
        CreateDemoMaterials();
        SetupScene();
        SetupLights();
        
        return true;
    }
    
    void Update(float deltaTime) override {
        m_world->Update(deltaTime);
        
        // Animate lights for demo
        AnimateLights(deltaTime);
        
        // Handle input
        HandleInput();
    }
    
    void OnKeyPressed(int key) override {
        switch (key) {
            case 49: // '1' - Toggle raytracing
                m_raytracingEnabled = !m_raytracingEnabled;
                m_lighting->EnableRaytracing(m_raytracingEnabled);
                break;
            case 50: // '2' - Toggle global illumination
                m_globalIllumination = !m_globalIllumination;
                m_lighting->EnableGlobalIllumination(m_globalIllumination);
                break;
            case 51: // '3' - Cycle quality levels
                m_qualityLevel = (m_qualityLevel + 1) % 4;
                m_lighting->SetQualityLevel(m_qualityLevel);
                break;
            case 52: // '4' - Add random light
                AddRandomLight();
                break;
            case 53: // '5' - Clear all lights
                m_lighting->ClearLights();
                SetupLights();
                break;
            case 82: // 'R' - Reset scene
                m_world->Clear();
                SetupScene();
                break;
        }
    }
    
    void OnMousePressed(int button, float x, float y) override {
        if (button == 0) { // Left click - place fire
            PlaceMaterial(static_cast<int>(x), static_cast<int>(y), m_fireMaterial, 3);
        }
        else if (button == 1) { // Right click - place light
            AddLightAtPosition(Vector2(x, y));
        }
    }

private:
    void CreateDemoMaterials() {
        // Glass material for refraction demo
        m_glassMaterial = m_materials->CreateMaterial("Glass")
            .SetColor(200, 230, 255, 100)
            .SetBehavior(MaterialBehavior::Static)
            .GetID();
        
        // Metal material for reflection demo
        m_metalMaterial = m_materials->CreateMaterial("Metal")
            .SetColor(150, 150, 150)
            .SetBehavior(MaterialBehavior::Static)
            .GetID();
        
        // Fire material with strong emission
        m_fireMaterial = m_materials->CreateMaterial("Fire")
            .SetColor(255, 100, 0)
            .SetBehavior(MaterialBehavior::Fire)
            .SetEmission(5.0f)
            .GetID();
        
        // Water with refraction
        m_waterMaterial = m_materials->CreateMaterial("Water")
            .SetColor(64, 164, 223, 150)
            .SetBehavior(MaterialBehavior::Liquid)
            .GetID();
        
        // Smoke for volumetric effects
        m_smokeMaterial = m_materials->CreateMaterial("Smoke")
            .SetColor(100, 100, 100, 80)
            .SetBehavior(MaterialBehavior::Gas)
            .GetID();
    }
    
    void SetupScene() {
        int width = m_world->GetWidth();
        int height = m_world->GetHeight();
        
        // Create ground
        for (int x = 0; x < width; ++x) {
            for (int y = height - 50; y < height; ++y) {
                m_world->SetMaterial(x, y, m_materials->GetMaterialID("Stone"));
            }
        }
        
        // Create glass prisms for refraction
        CreateGlassPrism(200, height - 200, 50, 100);
        CreateGlassPrism(600, height - 150, 30, 80);
        
        // Create metal mirrors
        CreateMetalMirror(400, height - 250, 5, 150);
        CreateMetalMirror(800, height - 200, 5, 120);
        
        // Create water pool
        CreateWaterPool(300, height - 100, 200, 50);
        
        // Add some fire sources
        CreateFireSource(100, height - 100, 10);
        CreateFireSource(700, height - 100, 15);
    }
    
    void SetupLights() {
        // Main sunlight (directional)
        Light sunlight;
        sunlight.type = LightType::Directional;
        sunlight.position = Vector2(m_world->GetWidth() / 2, 0);
        sunlight.direction = Vector2(0.3f, 1.0f).Normalized();
        sunlight.color = Vector3(1.0f, 0.95f, 0.8f);
        sunlight.intensity = 2.0f;
        sunlight.radius = 1000.0f;
        m_lighting->AddLight(sunlight);
        
        // Colored point lights
        Light redLight;
        redLight.type = LightType::Point;
        redLight.position = Vector2(150, m_world->GetHeight() - 300);
        redLight.color = Vector3(1.0f, 0.2f, 0.1f);
        redLight.intensity = 3.0f;
        redLight.radius = 200.0f;
        m_lightIds.push_back(m_lighting->AddLight(redLight));
        
        Light blueLight;
        blueLight.type = LightType::Point;
        blueLight.position = Vector2(500, m_world->GetHeight() - 400);
        blueLight.color = Vector3(0.1f, 0.3f, 1.0f);
        blueLight.intensity = 3.0f;
        blueLight.radius = 180.0f;
        m_lightIds.push_back(m_lighting->AddLight(blueLight));
        
        Light greenLight;
        greenLight.type = LightType::Point;
        greenLight.position = Vector2(800, m_world->GetHeight() - 350);
        greenLight.color = Vector3(0.2f, 1.0f, 0.1f);
        greenLight.intensity = 2.5f;
        greenLight.radius = 160.0f;
        m_lightIds.push_back(m_lighting->AddLight(greenLight));
    }
    
    void AnimateLights(float deltaTime) {
        m_lightAnimTime += deltaTime;
        
        // Animate the colored lights in circular patterns
        for (size_t i = 0; i < m_lightIds.size(); ++i) {
            float angle = m_lightAnimTime + i * 2.1f;
            float radius = 100.0f + sin(m_lightAnimTime * 0.5f + i) * 50.0f;
            
            Vector2 center(300 + i * 250, m_world->GetHeight() - 350);
            Vector2 newPos = center + Vector2(cos(angle) * radius, sin(angle) * radius * 0.5f);
            
            m_lighting->SetLightPosition(m_lightIds[i], newPos);
            
            // Pulse intensity
            float intensity = 2.0f + sin(m_lightAnimTime * 3.0f + i) * 1.0f;
            m_lighting->SetLightIntensity(m_lightIds[i], intensity);
        }
    }
    
    void CreateGlassPrism(int x, int y, int width, int height) {
        for (int dx = 0; dx < width; ++dx) {
            for (int dy = 0; dy < height; ++dy) {
                // Create triangular prism shape
                float ratio = static_cast<float>(dx) / width;
                int maxHeight = static_cast<int>(height * (1.0f - ratio));
                if (dy < maxHeight) {
                    m_world->SetMaterial(x + dx, y + dy, m_glassMaterial);
                }
            }
        }
    }
    
    void CreateMetalMirror(int x, int y, int width, int height) {
        for (int dx = 0; dx < width; ++dx) {
            for (int dy = 0; dy < height; ++dy) {
                m_world->SetMaterial(x + dx, y + dy, m_metalMaterial);
            }
        }
    }
    
    void CreateWaterPool(int x, int y, int width, int height) {
        for (int dx = 0; dx < width; ++dx) {
            for (int dy = 0; dy < height; ++dy) {
                m_world->SetMaterial(x + dx, y + dy, m_waterMaterial);
            }
        }
    }
    
    void CreateFireSource(int x, int y, int radius) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                if (dx * dx + dy * dy <= radius * radius) {
                    m_world->SetMaterial(x + dx, y + dy, m_fireMaterial);
                }
            }
        }
    }
    
    void AddRandomLight() {
        Light light;
        light.type = LightType::Point;
        light.position = Vector2(
            randf() * m_world->GetWidth(),
            randf() * m_world->GetHeight() * 0.7f
        );
        light.color = Vector3(randf(), randf(), randf()).Normalized();
        light.intensity = 2.0f + randf() * 3.0f;
        light.radius = 100.0f + randf() * 200.0f;
        
        m_lighting->AddLight(light);
    }
    
    void AddLightAtPosition(const Vector2& position) {
        Light light;
        light.type = LightType::Point;
        light.position = position;
        light.color = Vector3(1.0f, 1.0f, 1.0f);
        light.intensity = 3.0f;
        light.radius = 150.0f;
        
        m_lighting->AddLight(light);
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
    
    void HandleInput() {
        // Additional continuous input handling
    }
    
    float randf() {
        return static_cast<float>(rand()) / RAND_MAX;
    }
    
    SimulationWorld* m_world = nullptr;
    MaterialSystem* m_materials = nullptr;
    LightingSystem* m_lighting = nullptr;
    
    MaterialID m_glassMaterial = MATERIAL_EMPTY;
    MaterialID m_metalMaterial = MATERIAL_EMPTY;
    MaterialID m_fireMaterial = MATERIAL_EMPTY;
    MaterialID m_waterMaterial = MATERIAL_EMPTY;
    MaterialID m_smokeMaterial = MATERIAL_EMPTY;
    
    std::vector<LightHandle> m_lightIds;
    float m_lightAnimTime = 0.0f;
    
    bool m_raytracingEnabled = true;
    bool m_globalIllumination = true;
    int m_qualityLevel = 2;
};

int main() {
    EngineConfig config;
    config.appName = "BGE Lighting Demo";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.enableRaytracing = true;
    
    Engine& engine = Engine::Instance();
    if (!engine.Initialize(config)) {
        return -1;
    }
    
    auto app = std::make_unique<LightingDemoApp>();
    engine.Run(std::move(app));
    
    engine.Shutdown();
    return 0;
}