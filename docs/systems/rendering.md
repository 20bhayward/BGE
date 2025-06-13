# Rendering System

## Current State (Phase 1)

The rendering system currently provides basic functionality through a service-based architecture. It supports both Vulkan and OpenGL backends with a simple abstraction layer.

**Files**: `Renderer/Renderer.h/.cpp`, `Renderer/Vulkan/`, `Renderer/OpenGL/`

### Current Architecture

```cpp
class Renderer {
public:
    bool Initialize(Window* window);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void RenderWorld(SimulationWorld* world);
    void SetClearColor(float r, float g, float b, float a);
    
    // Basic rendering commands
    void DrawQuad(const Vector2& position, const Vector2& size, const Color& color);
    void DrawTexture(TextureID texture, const Vector2& position, const Vector2& size);
};
```

## Planned Enhancements (Phase 2)

### 1. Pixel-Perfect Rendering Pipeline

**Objective**: Create a rendering pipeline specifically optimized for pixel-art aesthetics with no filtering or anti-aliasing.

#### Implementation Plan

```cpp
class PixelArtRenderer {
public:
    struct PixelRenderSettings {
        bool enableFiltering = false;     // Always false for pixel art
        uint32_t pixelScale = 4;          // Integer scaling factor
        bool snapToPixels = true;         // Snap all positions to pixel boundaries
        bool enablePalettes = true;       // Support color palette systems
    };
    
    // Pixel-perfect camera
    class PixelCamera {
    public:
        void SetPosition(const Vector2& pos);
        Vector2 GetPosition() const;
        
        void SetZoom(float zoom);         // Must be integer multiples
        float GetZoom() const;
        
        Matrix4 GetViewMatrix() const;
        Matrix4 GetProjectionMatrix() const;
        
    private:
        Vector2 m_position{0, 0};
        float m_zoom = 1.0f;
        bool m_snapToPixels = true;
    };
    
    // Sprite rendering with pixel-perfect positioning
    void DrawSprite(const Sprite& sprite, const Vector2& position, 
                   uint32_t paletteIndex = 0);
    
    // Tilemap rendering optimized for large worlds
    void DrawTilemap(const Tilemap& tilemap, const Vector2& offset);
    
    // Material/voxel rendering for simulation
    void DrawMaterialGrid(const MaterialGrid& grid, const Vector2& offset);
};
```

#### Shader System for Pixel Art

```glsl
// Vertex Shader - pixel_perfect.vert
#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in uint paletteIndex;

uniform mat4 viewProjection;
uniform float pixelScale;

out vec2 fragTexCoord;
flat out uint fragPaletteIndex;

void main() {
    // Snap to pixel grid
    vec2 snappedPos = floor(position * pixelScale) / pixelScale;
    gl_Position = viewProjection * vec4(snappedPos, 0.0, 1.0);
    
    fragTexCoord = texCoord;
    fragPaletteIndex = paletteIndex;
}
```

```glsl
// Fragment Shader - pixel_perfect.frag
#version 450 core

in vec2 fragTexCoord;
flat in uint fragPaletteIndex;

uniform sampler2D spriteTexture;
uniform sampler2D paletteTexture;
uniform bool enablePalettes;

out vec4 fragColor;

void main() {
    // Sample sprite texture (no filtering)
    vec4 spriteColor = texture(spriteTexture, fragTexCoord);
    
    if (enablePalettes && spriteColor.r > 0.0) {
        // Use red channel as palette index
        float paletteCoord = (spriteColor.r * 255.0 + 0.5) / 256.0;
        vec2 paletteUV = vec2(paletteCoord, float(fragPaletteIndex) / 256.0);
        fragColor = texture(paletteTexture, paletteUV);
    } else {
        fragColor = spriteColor;
    }
    
    // Discard transparent pixels
    if (fragColor.a < 0.1) {
        discard;
    }
}
```

### 2. Enhanced Lighting System

#### 2D Raytracing Improvements

```cpp
class Raytracer2D {
public:
    struct LightSource {
        Vector2 position;
        Color color = Color::WHITE;
        float intensity = 1.0f;
        float radius = 100.0f;
        LightType type = LightType::POINT;
        bool castsShadows = true;
        
        // Advanced properties
        float falloffExponent = 2.0f;     // Quadratic falloff by default
        float innerCone = 30.0f;          // For spot lights (degrees)
        float outerCone = 45.0f;          // For spot lights (degrees)
        Vector2 direction = {0, -1};      // For directional/spot lights
        
        // Dynamic properties
        bool flickers = false;
        float flickerRate = 2.0f;
        float flickerAmount = 0.1f;
        
        // Temperature simulation (affects color)
        float temperature = 6500.0f;     // Kelvin (daylight = ~6500K)
    };
    
    struct ShadowCaster {
        std::vector<Vector2> vertices;
        bool isClosed = true;
        float opacity = 1.0f;
    };
    
    // Advanced lighting calculation
    void CalculateLighting(const std::vector<LightSource>& lights,
                          const std::vector<ShadowCaster>& shadowCasters,
                          LightingGrid& outputGrid);
    
    // Color temperature conversion
    Color TemperatureToColor(float kelvin);
    
    // Volumetric lighting effects
    void CalculateVolumetricLighting(const LightSource& light,
                                   const MaterialGrid& materials,
                                   VolumetricGrid& output);
    
private:
    // Optimized ray casting using spatial acceleration
    struct LightingAcceleration {
        std::vector<SpatialHash> lightHashes;
        std::vector<SpatialHash> shadowHashes;
        OcclusionMap occlusionMap;
    };
    
    // Multi-threaded lighting calculation
    void CalculateLightingTiled(const LightingParams& params,
                               const TileRegion& region,
                               LightingGrid& output);
};
```

#### Lighting Integration with Materials

```cpp
// Material lighting properties
struct MaterialLightingData {
    float reflectivity = 0.1f;      // How much light is reflected
    float absorption = 0.9f;        // How much light is absorbed
    float emission = 0.0f;          // Self-emitted light
    Color emissionColor = Color::WHITE;
    
    // Advanced properties
    float roughness = 0.5f;         // Surface roughness (affects reflections)
    float metallic = 0.0f;          // Metallic reflection properties
    float transmission = 0.0f;      // For transparent/translucent materials
    
    // Temperature-based emission
    bool temperatureEmission = false;
    float emissionThreshold = 500.0f; // Temperature to start glowing (Celsius)
};

class MaterialLightingSystem {
public:
    void UpdateMaterialLighting(MaterialGrid& materials, 
                               LightingGrid& lighting,
                               float deltaTime) {
        for (uint32_t y = 0; y < materials.height; ++y) {
            for (uint32_t x = 0; x < materials.width; ++x) {
                auto material = materials.GetMaterial(x, y);
                auto& lightData = GetMaterialLighting(material);
                
                // Temperature-based emission
                if (lightData.temperatureEmission) {
                    float temp = materials.GetTemperature(x, y);
                    if (temp > lightData.emissionThreshold) {
                        float emission = CalculateTemperatureEmission(temp);
                        Color emissionColor = TemperatureToColor(temp);
                        lighting.AddEmission(x, y, emission, emissionColor);
                    }
                }
                
                // Light scattering in gases
                if (IsGas(material)) {
                    float density = materials.GetDensity(x, y);
                    lighting.ScatterLight(x, y, density * lightData.absorption);
                }
            }
        }
    }
    
private:
    float CalculateTemperatureEmission(float temperature) {
        // Stefan-Boltzmann law approximation for visible light
        float normalizedTemp = (temperature - 500.0f) / 1000.0f;
        return std::clamp(normalizedTemp * normalizedTemp, 0.0f, 1.0f);
    }
};
```

### 3. Particle System

```cpp
class ParticleSystem {
public:
    struct Particle {
        Vector2 position;
        Vector2 velocity;
        Vector2 acceleration;
        
        Color startColor = Color::WHITE;
        Color endColor = Color::TRANSPARENT;
        float colorBlendFactor = 0.0f;
        
        float size = 1.0f;
        float startSize = 1.0f;
        float endSize = 0.0f;
        
        float lifetime = 1.0f;
        float age = 0.0f;
        
        // Physics properties
        float mass = 1.0f;
        float drag = 0.99f;
        float bounce = 0.5f;
        
        // Rendering properties
        BlendMode blendMode = BlendMode::ADDITIVE;
        TextureID texture = 0;
        bool worldSpace = true;
        
        // Custom data for specialized particles
        float customData[4] = {0, 0, 0, 0};
    };
    
    struct ParticleEmitter {
        Vector2 position;
        Vector2 direction = {0, 1};
        float angle = 0.0f;              // Cone angle for emission
        
        float emissionRate = 10.0f;       // Particles per second
        uint32_t maxParticles = 100;
        
        // Particle property ranges
        Range<float> lifetime = {1.0f, 2.0f};
        Range<float> speed = {50.0f, 100.0f};
        Range<Vector2> acceleration = {{0, -98}, {0, -98}}; // Gravity
        Range<Color> startColor = {Color::WHITE, Color::WHITE};
        Range<Color> endColor = {Color::TRANSPARENT, Color::TRANSPARENT};
        Range<float> size = {1.0f, 3.0f};
        
        // Emitter behavior
        bool continuous = true;
        float burstSize = 0.0f;          // 0 = continuous, >0 = burst mode
        float burstInterval = 1.0f;
        
        // Physics interaction
        bool collideWithWorld = true;
        bool affectedByWind = false;
        MaterialID collisionMaterial = MATERIAL_EMPTY;
    };
    
    // Core functionality
    EmitterID CreateEmitter(const ParticleEmitter& config);
    void DestroyEmitter(EmitterID id);
    void UpdateEmitter(EmitterID id, const ParticleEmitter& config);
    
    void Update(float deltaTime);
    void Render(const Matrix4& viewProjection);
    
    // Specialized effects
    EmitterID CreateSparks(const Vector2& position, float intensity);
    EmitterID CreateSmoke(const Vector2& position, const Color& color);
    EmitterID CreateFire(const Vector2& position, float size);
    EmitterID CreateSteam(const Vector2& position, float amount);
    EmitterID CreateBlood(const Vector2& position, const Color& bloodColor);
    
    // Batch operations
    void CreateExplosion(const Vector2& position, float radius, uint32_t particleCount);
    void CreateWeatherEffect(WeatherType type, const Bounds2D& area);
    
private:
    struct ParticleBatch {
        std::vector<Particle> particles;
        ParticleEmitter emitter;
        float nextEmissionTime = 0.0f;
        bool active = true;
    };
    
    std::unordered_map<EmitterID, ParticleBatch> m_emitters;
    EmitterID m_nextEmitterID = 1;
    
    // Rendering resources
    struct ParticleRenderData {
        VertexBuffer vertexBuffer;
        IndexBuffer indexBuffer;
        ShaderProgram shader;
        uint32_t maxVertices = 10000;
    } m_renderData;
    
    // Physics integration
    void UpdateParticlePhysics(Particle& particle, float deltaTime);
    bool CheckWorldCollision(const Vector2& position, const Vector2& velocity);
};
```

### 4. Screen-Space Effects

```cpp
class PostProcessSystem {
public:
    struct ScreenShakeData {
        float intensity = 0.0f;
        float duration = 0.0f;
        float frequency = 30.0f;
        Vector2 direction = {1, 1};
        bool dampening = true;
    };
    
    struct BloomSettings {
        bool enabled = false;
        float threshold = 1.0f;      // Brightness threshold
        float intensity = 0.5f;      // Effect intensity
        uint32_t iterations = 5;     // Blur iterations
        float scatter = 0.7f;        // Light scattering amount
    };
    
    struct ColorGrading {
        float exposure = 1.0f;
        float contrast = 1.0f;
        float saturation = 1.0f;
        Vector3 colorBalance = {1, 1, 1}; // RGB multipliers
        
        // Tone mapping
        ToneMappingType toneMapping = ToneMappingType::REINHARD;
        float whitePoint = 1.0f;
    };
    
    // Core post-processing pipeline
    void BeginPostProcess(const RenderTarget& sceneTarget);
    void ApplyBloom(const BloomSettings& settings);
    void ApplyScreenShake(const ScreenShakeData& shake, Matrix4& viewMatrix);
    void ApplyColorGrading(const ColorGrading& grading);
    void EndPostProcess(const RenderTarget& finalTarget);
    
    // Effect management
    void AddScreenShake(float intensity, float duration);
    void SetBloomSettings(const BloomSettings& settings);
    void SetColorGrading(const ColorGrading& grading);
    
    // Transition effects
    void FadeToBlack(float duration, std::function<void()> callback = nullptr);
    void FadeFromBlack(float duration);
    void CrossFade(TextureID targetTexture, float duration);
    
private:
    // Render targets for multi-pass effects
    RenderTarget m_sceneTarget;
    RenderTarget m_bloomTarget;
    RenderTarget m_tempTargets[2];
    
    // Shader programs
    ShaderProgram m_bloomShader;
    ShaderProgram m_blurShader;
    ShaderProgram m_compositeShader;
    ShaderProgram m_colorGradingShader;
    
    // Current effect states
    ScreenShakeData m_currentShake;
    BloomSettings m_bloomSettings;
    ColorGrading m_colorGrading;
    
    // Transition state
    struct TransitionState {
        bool active = false;
        float progress = 0.0f;
        float duration = 1.0f;
        TransitionType type = TransitionType::FADE;
        std::function<void()> callback;
    } m_transition;
};
```

## Rendering Architecture Overview

### Service Integration

```cpp
// Renderer service registration
void Engine::RegisterCoreServices() {
    auto renderer = std::make_shared<PixelArtRenderer>();
    if (renderer->Initialize(m_window.get())) {
        ServiceLocator::Instance().RegisterService<Renderer>(renderer);
        ServiceLocator::Instance().RegisterService<PixelArtRenderer>(renderer);
    }
    
    auto particleSystem = std::make_shared<ParticleSystem>();
    particleSystem->Initialize(renderer.get());
    ServiceLocator::Instance().RegisterService<ParticleSystem>(particleSystem);
    
    auto postProcess = std::make_shared<PostProcessSystem>();
    postProcess->Initialize(renderer.get());
    ServiceLocator::Instance().RegisterService<PostProcessSystem>(postProcess);
}

// Usage in application
class BlacksmithingApp : public Application {
public:
    void Update(float deltaTime) override {
        // Create sparks when hammering
        if (m_hammerStrike) {
            auto particles = Services::Get<ParticleSystem>();
            particles->CreateSparks(m_hammerPosition, m_hammerForce);
            
            auto postProcess = Services::Get<PostProcessSystem>();
            postProcess->AddScreenShake(m_hammerForce * 0.1f, 0.2f);
        }
    }
};
```

### Event-Driven Rendering

```cpp
// Subscribe to game events for visual effects
EventBus::Instance().Subscribe<ForgeOperationEvent>([](const ForgeOperationEvent& event) {
    auto particles = Services::Get<ParticleSystem>();
    auto postProcess = Services::Get<PostProcessSystem>();
    
    // Visual effects based on operation type
    switch (event.operation) {
        case ForgeOperation::DRAW_OUT:
            particles->CreateSparks(event.position, event.force);
            break;
            
        case ForgeOperation::QUENCH:
            particles->CreateSteam(event.position, event.temperature / 100.0f);
            break;
            
        case ForgeOperation::WELD:
            particles->CreateFire(event.position, 2.0f);
            postProcess->AddScreenShake(event.force * 0.2f, 0.5f);
            break;
    }
});

EventBus::Instance().Subscribe<MaterialReactionEvent>([](const MaterialReactionEvent& event) {
    auto particles = Services::Get<ParticleSystem>();
    
    // Different reactions create different effects
    if (event.energyReleased > 10.0f) {
        particles->CreateExplosion(Vector2{event.x, event.y}, 
                                 event.energyReleased, 
                                 static_cast<uint32_t>(event.energyReleased * 5));
    }
});
```

### Performance Considerations

#### Batched Rendering
```cpp
class BatchRenderer {
public:
    void BeginBatch();
    void DrawSprite(const Sprite& sprite, const Vector2& position, float rotation = 0.0f);
    void DrawMaterial(MaterialID material, const Vector2& position);
    void EndBatch();
    
private:
    struct SpriteVertex {
        Vector2 position;
        Vector2 texCoord;
        Color color;
        float textureIndex;
    };
    
    static constexpr uint32_t MAX_SPRITES = 10000;
    static constexpr uint32_t MAX_TEXTURES = 32;
    
    std::array<SpriteVertex, MAX_SPRITES * 4> m_vertices;
    std::array<uint32_t, MAX_SPRITES * 6> m_indices;
    std::array<TextureID, MAX_TEXTURES> m_textures;
    
    uint32_t m_spriteCount = 0;
    uint32_t m_textureCount = 0;
};
```

#### Level-of-Detail for Large Worlds
```cpp
class LODRenderer {
public:
    void SetViewport(const Bounds2D& viewport);
    void RenderWorld(const SimulationWorld& world);
    
private:
    enum class LODLevel {
        FULL_DETAIL = 0,    // Every pixel
        HALF_DETAIL = 1,    // Every 2nd pixel
        QUARTER_DETAIL = 2, // Every 4th pixel
        CHUNK_SUMMARY = 3   // Chunk average colors
    };
    
    LODLevel CalculateLOD(const Vector2& position, const Vector2& viewCenter, float zoom);
    void RenderChunkAtLOD(const Chunk& chunk, LODLevel lod);
};
```

## Future Extensions

### Phase 3 Features
1. **3D Lighting Simulation**: Full 3D light propagation for more realistic shadows
2. **Deferred Rendering**: Support for many dynamic lights
3. **Procedural Animations**: Data-driven sprite animation system
4. **Atmospheric Scattering**: Realistic daylight simulation
5. **Real-time Reflections**: Water and metal surface reflections
6. **Advanced Post-Processing**: Motion blur, depth of field, temporal anti-aliasing

### Modding Support
```cpp
// Plugin system for custom renderers
class IRenderPlugin {
public:
    virtual ~IRenderPlugin() = default;
    virtual void Initialize(Renderer* renderer) = 0;
    virtual void Render(const RenderContext& context) = 0;
    virtual std::string GetName() const = 0;
};

// Custom shader support
class ShaderManager {
public:
    bool LoadShaderPack(const std::string& packPath);
    ShaderID CreateCustomShader(const std::string& vertexSource, 
                               const std::string& fragmentSource);
    void ReloadShaders(); // Hot-reloading for development
};
```