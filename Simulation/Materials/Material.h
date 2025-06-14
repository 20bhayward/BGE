#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "OpticalProperties.h"

namespace BGE {

using MaterialID = uint16_t;
constexpr MaterialID MATERIAL_EMPTY = 0;

enum class MaterialState : uint8_t {
    Solid,
    Liquid,
    Gas,
    Plasma
};

enum class MaterialBehavior : uint8_t {
    Static,      // Does not move (rock, metal)
    Powder,      // Falls and piles (sand, dirt)
    Liquid,      // Flows and spreads (water, oil)
    Gas,         // Disperses upward (steam, smoke)
    Fire         // Special behavior for combustion
};

enum class VisualPattern : uint8_t {
    Solid,          // Standard solid color
    Speck,          // Small random colored specks
    Wavy,           // Wavy lines pattern
    Line,           // Straight line pattern
    Border,         // Different color on border
    Gradient,       // Vertical/horizontal gradient
    Checkerboard,   // Checkerboard pattern
    Dots,           // Regular dot pattern
    Stripes,        // Diagonal stripes
    Noise,          // Random noise texture
    Marble,         // Marble-like veins
    Crystal,        // Crystal facet pattern
    Honeycomb,      // Hexagonal honeycomb
    Spiral,         // Spiral pattern
    Ripple,         // Concentric ripples
    Flame,          // Flame-like pattern
    Wood,           // Wood grain pattern
    Metal,          // Metallic brushed pattern
    Fabric,         // Fabric weave pattern
    Scale,          // Dragon scale pattern
    Bubble,         // Bubble texture
    Crack,          // Cracked surface
    Flow,           // Flowing liquid pattern
    Spark,          // Sparkling particles
    Glow,           // Glowing edges
    Frost,          // Frost crystal pattern
    Sand,           // Sand grain texture
    Rock,           // Rocky surface
    Plasma,         // Plasma energy pattern
    Lightning,      // Lightning bolts
    Smoke,          // Smoke wisps
    Steam,          // Steam condensation
    Oil,            // Oil slick rainbow
    Blood,          // Blood droplet pattern
    Acid,           // Acid bubbling
    Ice,            // Ice crystal formation
    Lava,           // Lava flow pattern
    Gas,            // Gas particle movement
    Liquid,         // Liquid surface tension
    Powder          // Powder grain pattern
};

struct VisualProperties {
    VisualPattern pattern = VisualPattern::Solid;
    uint32_t secondaryColor = 0xFF000000; // For patterns requiring second color
    float patternScale = 1.0f;            // Scale of pattern elements
    float patternIntensity = 0.5f;        // How prominent the pattern is
    float animationSpeed = 0.0f;          // Speed of pattern animation
};

struct ThermalProperties {
    float meltingPoint = 1000.0f;    // Temperature to change to liquid
    float boilingPoint = 2000.0f;    // Temperature to change to gas
    float ignitionPoint = 500.0f;    // Temperature to start burning
    float thermalConductivity = 1.0f; // How fast heat spreads
    float heatCapacity = 1.0f;       // How much heat it can store
};

struct PhysicalProperties {
    float density = 1.0f;            // Affects settling and displacement
    float viscosity = 0.0f;          // Flow resistance (liquids)
    float friction = 0.5f;           // Surface friction
    float corrosion = 0.0f;          // How much it corrodes other materials
    float hardness = 1.0f;           // Resistance to displacement
};

struct MaterialReaction {
    MaterialID reactant;             // What material this reacts with
    MaterialID product1;             // First product
    MaterialID product2 = MATERIAL_EMPTY; // Optional second product
    float probability = 1.0f;        // Chance of reaction (0-1)
    float energyChange = 0.0f;       // Heat generated/consumed
    bool requiresHeat = false;       // Needs minimum temperature
    float minTemperature = 0.0f;     // Minimum temp for reaction
};

class Material {
public:
    Material() = default;
    Material(MaterialID id, const std::string& name);
    
    // Basic properties
    MaterialID GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    
    MaterialState GetState() const { return m_state; }
    MaterialBehavior GetBehavior() const { return m_behavior; }
    
    // Visual properties
    uint32_t GetColor() const { return m_color; }
    void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    
    // Physics properties
    const PhysicalProperties& GetPhysicalProps() const { return m_physicalProps; }
    const ThermalProperties& GetThermalProps() const { return m_thermalProps; }
    const OpticalProperties& GetOpticalProps() const { return m_opticalProps; }
    const VisualProperties& GetVisualProps() const { return m_visualProps; }
    
    // Reactions
    void AddReaction(const MaterialReaction& reaction);
    const std::vector<MaterialReaction>& GetReactions() const { return m_reactions; }
    
    // Builder pattern for easy material creation
    Material& SetState(MaterialState state) { m_state = state; return *this; }
    Material& SetBehavior(MaterialBehavior behavior) { m_behavior = behavior; return *this; }
    Material& SetDensity(float density) { m_physicalProps.density = density; return *this; }
    Material& SetMeltingPoint(float temp) { m_thermalProps.meltingPoint = temp; return *this; }
    Material& SetEmission(float emission) { m_opticalProps.emission = emission; return *this; }
    Material& SetVisualPattern(VisualPattern pattern) { m_visualProps.pattern = pattern; return *this; }
    Material& SetSecondaryColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { 
        m_visualProps.secondaryColor = (a << 24) | (r << 16) | (g << 8) | b; 
        return *this; 
    }
    
    // Hotkey
    int GetHotKey() const { return m_hotkey; }

    friend class MaterialSystem; // Allow MaterialSystem and its nested MaterialBuilder to access private members

private:
    int m_hotkey = 0; // Hotkey for palette selection
    MaterialID m_id = MATERIAL_EMPTY;
    std::string m_name;
    
    MaterialState m_state = MaterialState::Solid;
    MaterialBehavior m_behavior = MaterialBehavior::Static;
    
    uint32_t m_color = 0xFF000000; // RGBA
    
    PhysicalProperties m_physicalProps;
    ThermalProperties m_thermalProps;
    OpticalProperties m_opticalProps;
    VisualProperties m_visualProps;
    
    std::vector<MaterialReaction> m_reactions;
};

} // namespace BGE