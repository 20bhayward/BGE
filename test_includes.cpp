#include "Core/Engine.h"
#include "Core/Application.h"
#include "Simulation/SimulationWorld.h"
#include "Simulation/Materials/MaterialSystem.h"

// Simple test to verify headers compile correctly
int main() {
    // Test that basic types are available
    BGE::MaterialID testId = BGE::MATERIAL_EMPTY;
    BGE::MaterialBehavior testBehavior = BGE::MaterialBehavior::Static;
    
    // Test that classes can be instantiated
    BGE::EngineConfig config;
    config.appName = "Test";
    
    return 0;
}