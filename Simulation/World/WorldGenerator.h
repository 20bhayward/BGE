#pragma once

#include "../Materials/Material.h"

namespace BGE {

class SimulationWorld;

class WorldGenerator {
public:
    WorldGenerator();
    ~WorldGenerator();
    
    // Generation methods
    void GenerateFlat(SimulationWorld* world, int groundLevel, MaterialID groundMaterial);
    void GenerateTerrain(SimulationWorld* world, int baseHeight, int variation);
    void GenerateCaves(SimulationWorld* world, float density);
    void AddRandomStructures(SimulationWorld* world, int count);
    
    // Noise-based generation
    void GeneratePerlinTerrain(SimulationWorld* world, float scale, float amplitude);
    void GenerateVegetation(SimulationWorld* world, float density);
    
    // Settings
    void SetSeed(uint32_t seed) { m_seed = seed; }
    uint32_t GetSeed() const { return m_seed; }

private:
    uint32_t m_seed;
    
    // Noise functions
    float PerlinNoise(float x, float y) const;
    float Random(int x, int y) const;
    float Interpolate(float a, float b, float t) const;
    
    // Generation helpers
    void PlaceStructure(SimulationWorld* world, int x, int y, const char* structureType);
    MaterialID GetTerrainMaterial(int height, int maxHeight) const;
};

} // namespace BGE