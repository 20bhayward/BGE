#pragma once

#include <vector>
#include <memory>
#include <array>
#include "Light.h"
#include "Core/Math/Vector2.h"
#include "Simulation/Materials/OpticalProperties.h"

namespace BGE {

class SimulationWorld;
class ComputeBuffer;
class RenderDevice;

struct Ray2D {
    Vector2 origin;
    Vector2 direction;
    Vector3 color;
    float intensity = 1.0f;
    int bounces = 0;
    float distance = 0.0f;
};

struct RayHit {
    bool hit = false;
    Vector2 position;
    Vector2 normal;
    MaterialID material = MATERIAL_EMPTY;
    float distance = 0.0f;
    OpticalProperties properties;
};

struct LightSample {
    Vector3 radiance;
    float distance;
    Vector2 direction;
};

class Raytracer2D {
public:
    explicit Raytracer2D(RenderDevice* device);
    ~Raytracer2D();
    
    // Core raytracing
    void TraceFrame(SimulationWorld* world, const std::vector<Light>& lights);
    void TraceLighting(SimulationWorld* world, const std::vector<Light>& lights);
    
    // Ray operations
    bool TraceRay(const Ray2D& ray, SimulationWorld* world, RayHit& hit);
    Vector3 SampleLighting(const Vector2& position, const Vector2& normal, 
                          SimulationWorld* world, const std::vector<Light>& lights);
    
    // Global illumination
    Vector3 ComputeGlobalIllumination(const Vector2& position, const Vector2& normal,
                                     SimulationWorld* world, const std::vector<Light>& lights,
                                     int maxBounces = 3);
    
    // Light map access
    const float* GetLightMap() const { return m_lightMap.data(); }
    uint32_t GetLightMapWidth() const { return m_lightMapWidth; }
    uint32_t GetLightMapHeight() const { return m_lightMapHeight; }
    
    // Settings
    void SetMaxBounces(int bounces) { m_maxBounces = bounces; }
    void SetRayDensity(float density) { m_rayDensity = density; }
    void SetLightMapResolution(uint32_t width, uint32_t height);
    void SetQualityLevel(int level); // 0-3, higher = better quality
    
    // Performance options
    void EnableGPUAcceleration(bool enable) { m_useGPU = enable; }
    void EnableTemporalAccumulation(bool enable) { m_temporalAccumulation = enable; }
    void SetSampleCount(int samples) { m_sampleCount = samples; }
    
    // Denoising
    void EnableDenoising(bool enable) { m_enableDenoising = enable; }
    void ApplyDenoising();
    
    // Debug visualization
    void EnableDebugRays(bool enable) { m_debugRays = enable; }
    const std::vector<Ray2D>& GetDebugRays() const { return m_debugRayData; }

private:
    // CPU raytracing implementation
    void TraceCPU(SimulationWorld* world, const std::vector<Light>& lights);
    void TraceRaysFromLight(const Light& light, SimulationWorld* world);
    void PropagateLight(const Vector2& position, const Vector3& color, 
                       SimulationWorld* world, int depth = 0);
    
    // GPU raytracing implementation  
    void TraceGPU(SimulationWorld* world, const std::vector<Light>& lights);
    void SetupComputeBuffers(SimulationWorld* world);
    void DispatchComputeShaders();
    
    // Material interaction
    Vector3 ProcessMaterialInteraction(const Ray2D& ray, const RayHit& hit, 
                                      SimulationWorld* world);
    Vector3 CalculateReflection(const Vector2& incident, const Vector2& normal, 
                               const OpticalProperties& props);
    Vector3 CalculateRefraction(const Vector2& incident, const Vector2& normal,
                               float refractionIndex);
    Vector3 CalculateScattering(const Vector2& position, const OpticalProperties& props);
    
    // Sampling and filtering
    std::vector<Vector2> GenerateRaySamples(const Light& light);
    void FilterLightMap();
    void BlendTemporalSamples();
    
    // Optimization techniques
    void UpdateSpatialCache(SimulationWorld* world);
    bool CanSkipPixel(int x, int y, SimulationWorld* world);
    void UpdateLightBounds(const std::vector<Light>& lights);
    
    RenderDevice* m_device;
    
    // Light map storage
    std::vector<float> m_lightMap;        // RGB per pixel
    std::vector<float> m_previousFrame;   // For temporal accumulation
    uint32_t m_lightMapWidth = 0;
    uint32_t m_lightMapHeight = 0;
    
    // GPU resources
    std::unique_ptr<ComputeBuffer> m_worldBuffer;
    std::unique_ptr<ComputeBuffer> m_lightBuffer;
    std::unique_ptr<ComputeBuffer> m_lightMapBuffer;
    std::unique_ptr<ComputeBuffer> m_materialPropsBuffer;
    
    // Settings
    int m_maxBounces = 3;
    float m_rayDensity = 1.0f;
    int m_sampleCount = 16;
    bool m_useGPU = true;
    bool m_temporalAccumulation = true;
    bool m_enableDenoising = true;
    
    // Quality settings
    int m_qualityLevel = 2;
    float m_minRayIntensity = 0.01f;
    float m_maxRayDistance = 1000.0f;
    
    // Performance tracking
    uint64_t m_frameCount = 0;
    float m_lastTraceTime = 0.0f;
    
    // Debug data
    bool m_debugRays = false;
    std::vector<Ray2D> m_debugRayData;
    
    // Spatial optimization
    struct LightCell {
        std::vector<int> lightIndices;
        Vector3 averageColor;
        bool needsUpdate = true;
    };
    
    std::vector<LightCell> m_lightGrid;
    int m_gridWidth = 0, m_gridHeight = 0;
    int m_cellSize = 16;
    
    // Constants
    static constexpr float RAY_EPSILON = 0.001f;
    static constexpr int MAX_RAY_BOUNCES = 8;
    static constexpr float LIGHT_FALLOFF_THRESHOLD = 0.01f;
    static constexpr int DENOISING_KERNEL_SIZE = 3;
};

// Utility functions for 2D raytracing
namespace RayUtils {
    bool RayCircleIntersect(const Vector2& rayOrigin, const Vector2& rayDir,
                           const Vector2& circleCenter, float radius, float& t);
    
    bool RayLineIntersect(const Vector2& rayOrigin, const Vector2& rayDir,
                         const Vector2& lineStart, const Vector2& lineEnd, float& t);
    
    Vector2 ReflectVector(const Vector2& incident, const Vector2& normal);
    Vector2 RefractVector(const Vector2& incident, const Vector2& normal, float eta);
    
    float FresnelReflectance(const Vector2& incident, const Vector2& normal, float eta);
    Vector3 SampleHemisphere(const Vector2& normal, float roughness);
}

} // namespace BGE