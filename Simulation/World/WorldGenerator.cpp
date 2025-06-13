#include "WorldGenerator.h"
#include "../SimulationWorld.h"
#include "../Materials/MaterialSystem.h"
#include "../../Core/Math/Math.h"
#include <cmath>

namespace BGE {

WorldGenerator::WorldGenerator() : m_seed(12345) {
}

WorldGenerator::~WorldGenerator() = default;

void WorldGenerator::GenerateFlat(SimulationWorld* world, int groundLevel, MaterialID groundMaterial) {
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    
    for (uint32_t x = 0; x < width; ++x) {
        for (int y = groundLevel; y < static_cast<int>(height); ++y) {
            world->SetMaterial(x, y, groundMaterial);
        }
    }
}

void WorldGenerator::GenerateTerrain(SimulationWorld* world, int baseHeight, int variation) {
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    MaterialSystem* materials = world->GetMaterialSystem();
    
    MaterialID stoneMaterial = materials->GetMaterialID("Stone");
    MaterialID sandMaterial = materials->GetMaterialID("Sand");
    (void)sandMaterial; // TODO: Use this material in terrain generation
    
    if (stoneMaterial == MATERIAL_EMPTY) {
        stoneMaterial = materials->CreateMaterialBuilder("Stone")
            .SetColor(128, 128, 128)
            .SetBehavior(MaterialBehavior::Static)
            .GetID();
    }
    
    for (uint32_t x = 0; x < width; ++x) {
        // Generate height using simple noise
        float noiseValue = PerlinNoise(static_cast<float>(x) * 0.01f, 0.0f);
        int terrainHeight = baseHeight + static_cast<int>(noiseValue * variation);
        terrainHeight = Math::Clamp(terrainHeight, 0, static_cast<int>(height - 1));
        
        for (int y = terrainHeight; y < static_cast<int>(height); ++y) {
            MaterialID material = GetTerrainMaterial(y - terrainHeight, 50);
            world->SetMaterial(x, y, material);
        }
    }
}

void WorldGenerator::GenerateCaves(SimulationWorld* world, float density) {
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    
    for (uint32_t x = 0; x < width; ++x) {
        for (uint32_t y = 0; y < height; ++y) {
            if (world->GetMaterial(x, y) != MATERIAL_EMPTY) {
                float caveNoise = PerlinNoise(static_cast<float>(x) * 0.05f, static_cast<float>(y) * 0.05f);
                if (caveNoise > (1.0f - density)) {
                    world->SetMaterial(x, y, MATERIAL_EMPTY);
                }
            }
        }
    }
}

void WorldGenerator::AddRandomStructures(SimulationWorld* world, int count) {
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    
    for (int i = 0; i < count; ++i) {
        int x = Math::RandomInt(10, width - 10);
        int y = Math::RandomInt(10, height - 10);
        
        PlaceStructure(world, x, y, "Tree");
    }
}

void WorldGenerator::GeneratePerlinTerrain(SimulationWorld* world, float scale, float amplitude) {
    uint32_t width = world->GetWidth();
    uint32_t height = world->GetHeight();
    MaterialSystem* materials = world->GetMaterialSystem();
    
    MaterialID groundMaterial = materials->GetMaterialID("Sand");
    if (groundMaterial == MATERIAL_EMPTY) {
        groundMaterial = materials->CreateMaterialBuilder("Sand")
            .SetColor(194, 178, 128)
            .SetBehavior(MaterialBehavior::Powder)
            .GetID();
    }
    
    for (uint32_t x = 0; x < width; ++x) {
        float noise = PerlinNoise(static_cast<float>(x) * scale, 0.0f);
        int terrainHeight = static_cast<int>(height * 0.7f + noise * amplitude);
        terrainHeight = Math::Clamp(terrainHeight, 0, static_cast<int>(height - 1));
        
        for (int y = terrainHeight; y < static_cast<int>(height); ++y) {
            world->SetMaterial(x, y, groundMaterial);
        }
    }
}

void WorldGenerator::GenerateVegetation(SimulationWorld* world, float density) {
    (void)world; (void)density;
    // TODO: Implement vegetation generation
}

float WorldGenerator::PerlinNoise(float x, float y) const {
    // Simplified Perlin noise implementation
    int xi = static_cast<int>(x) & 255;
    int yi = static_cast<int>(y) & 255;
    
    float xf = x - static_cast<int>(x);
    float yf = y - static_cast<int>(y);
    
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);
    
    float n00 = Random(xi, yi);
    float n01 = Random(xi, yi + 1);
    float n10 = Random(xi + 1, yi);
    float n11 = Random(xi + 1, yi + 1);
    
    float x1 = Interpolate(n00, n10, u);
    float x2 = Interpolate(n01, n11, u);
    
    return Interpolate(x1, x2, v);
}

float WorldGenerator::Random(int x, int y) const {
    int n = x + y * 57 + m_seed;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float WorldGenerator::Interpolate(float a, float b, float t) const {
    float ft = t * Math::PI;
    float f = (1.0f - std::cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

void WorldGenerator::PlaceStructure(SimulationWorld* world, int x, int y, const char* structureType) {
    MaterialSystem* materials = world->GetMaterialSystem();
    
    if (strcmp(structureType, "Tree") == 0) {
        MaterialID woodMaterial = materials->GetMaterialID("Wood");
        if (woodMaterial == MATERIAL_EMPTY) {
            woodMaterial = materials->CreateMaterialBuilder("Wood")
                .SetColor(139, 69, 19)
                .SetBehavior(MaterialBehavior::Static)
                .GetID();
        }
        
        // Simple tree structure
        for (int dy = 0; dy < 8; ++dy) {
            if (world->IsValidPosition(x, y - dy)) {
                world->SetMaterial(x, y - dy, woodMaterial);
            }
        }
        
        // Tree crown
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -3; dy <= -1; ++dy) {
                if (world->IsValidPosition(x + dx, y + dy)) {
                    world->SetMaterial(x + dx, y + dy, woodMaterial);
                }
            }
        }
    }
}

MaterialID WorldGenerator::GetTerrainMaterial(int depth, int maxDepth) const {
    (void)maxDepth;
    // Simple depth-based material selection
    if (depth < 5) {
        return MATERIAL_EMPTY + 1; // Assuming sand is material ID 1
    } else if (depth < 20) {
        return MATERIAL_EMPTY + 2; // Assuming stone is material ID 2
    } else {
        return MATERIAL_EMPTY + 3; // Deep stone
    }
}

} // namespace BGE