#pragma once

namespace BGE {

struct OpticalProperties {
    // Light emission (for self-illuminating materials)
    float emission = 0.0f;          // Light emission intensity
    float emissionR = 1.0f;         // Red emission component
    float emissionG = 1.0f;         // Green emission component  
    float emissionB = 1.0f;         // Blue emission component
    
    // Light interaction
    float absorption = 0.1f;        // How much light is absorbed (0-1)
    float scattering = 0.0f;        // How much light scatters (0-1)
    float transmission = 0.0f;      // How much light passes through (0-1)
    
    // Surface properties
    float roughness = 0.5f;         // Surface roughness (affects reflections)
    float metallic = 0.0f;          // Metallic factor (affects reflectance)
    float refractionIndex = 1.0f;   // Index of refraction (for transparent materials)
    
    // Advanced properties for raytracing
    float subsurfaceScattering = 0.0f; // Light penetration depth
    float anisotropy = 0.0f;        // Directional scattering preference
    bool castsShadows = true;       // Whether material blocks light
    
    // Thermal radiation (heat-based emission)
    float thermalEmissionFactor = 0.0f; // How much heat becomes light
    float thermalEmissionThreshold = 500.0f; // Min temperature for thermal glow
    
    // Helper functions for common material types
    static OpticalProperties CreateMetal(float roughness = 0.1f) {
        OpticalProperties props;
        props.metallic = 1.0f;
        props.roughness = roughness;
        props.absorption = 0.95f;
        return props;
    }
    
    static OpticalProperties CreateGlass(float refractionIndex = 1.5f) {
        OpticalProperties props;
        props.transmission = 0.9f;
        props.absorption = 0.05f;
        props.refractionIndex = refractionIndex;
        props.roughness = 0.0f;
        return props;
    }
    
    static OpticalProperties CreateFire() {
        OpticalProperties props;
        props.emission = 2.0f;
        props.emissionR = 1.0f;
        props.emissionG = 0.6f;
        props.emissionB = 0.2f;
        props.scattering = 0.8f;
        props.thermalEmissionFactor = 1.0f;
        props.castsShadows = false;
        return props;
    }
    
    static OpticalProperties CreateWater() {
        OpticalProperties props;
        props.transmission = 0.7f;
        props.absorption = 0.2f;
        props.refractionIndex = 1.33f;
        props.scattering = 0.1f;
        props.roughness = 0.0f;
        return props;
    }
};

} // namespace BGE