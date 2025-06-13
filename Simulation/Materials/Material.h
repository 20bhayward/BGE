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
    
    // Reactions
    void AddReaction(const MaterialReaction& reaction);
    const std::vector<MaterialReaction>& GetReactions() const { return m_reactions; }
    
    // Builder pattern for easy material creation
    Material& SetState(MaterialState state) { m_state = state; return *this; }
    Material& SetBehavior(MaterialBehavior behavior) { m_behavior = behavior; return *this; }
    Material& SetDensity(float density) { m_physicalProps.density = density; return *this; }
    Material& SetMeltingPoint(float temp) { m_thermalProps.meltingPoint = temp; return *this; }
    Material& SetEmission(float emission) { m_opticalProps.emission = emission; return *this; }
    
    // Hotkey
    int GetHotKey() const { return m_hotkey; }

    friend class MaterialBuilder; // Allow MaterialBuilder to set m_hotkey

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
    
    std::vector<MaterialReaction> m_reactions;
};

} // namespace BGE