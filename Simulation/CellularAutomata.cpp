#include "CellularAutomata.h"
#include "SimulationWorld.h"
#include "Materials/MaterialSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace BGE {

// Define the static member variable
thread_local uint32_t CellularAutomata::s_randomState = 1;

CellularAutomata::CellularAutomata(SimulationWorld* world) 
    : m_world(world) {
}

CellularAutomata::~CellularAutomata() = default;

bool CellularAutomata::IsEmpty(int x, int y) const {
    return m_world->GetMaterial(x, y) == MATERIAL_EMPTY;
}

void CellularAutomata::ProcessReactions(int x, int y, float deltaTime) {
    (void)deltaTime; // Parameter available for future time-based reactions
    if (!m_world) return;
    
    MaterialID currentMaterial = m_world->GetMaterial(x, y);
    if (currentMaterial == MATERIAL_EMPTY) return;
    
    MaterialSystem* materialSystem = m_world->GetMaterialSystem();
    if (!materialSystem) return;
    
    const Material* mat = materialSystem->GetMaterialPtr(currentMaterial);
    if (!mat || mat->GetReactions().empty()) return;
    
    // Debug: Log when we find a material with reactions
    static int debugCount = 0;
    if (debugCount < 5 && !mat->GetReactions().empty()) {
        std::cout << "DEBUG: Material " << mat->GetName() << " has " << mat->GetReactions().size() << " reactions" << std::endl;
        debugCount++;
    }
    
    // Check all 8 neighboring cells for potential reactions
    static const std::array<std::pair<int, int>, 8> neighbors = {{
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    }};
    
    for (const auto& offset : neighbors) {
        int nx = x + offset.first;
        int ny = y + offset.second;
        
        if (!m_world->IsValidPosition(nx, ny)) continue;
        
        MaterialID neighborMaterial = m_world->GetMaterial(nx, ny);
        if (neighborMaterial == MATERIAL_EMPTY) continue;
        
        // Try reaction between current material and neighbor
        MaterialID product1, product2;
        if (materialSystem->ProcessReaction(currentMaterial, neighborMaterial, 20.0f, product1, product2)) {
            // Reaction occurred - check reaction type to determine how to handle it
            static int reactionCount = 0;
            if (reactionCount < 10) {
                std::cout << "DEBUG: Reaction " << reactionCount << " - " << mat->GetName() 
                         << " + neighbor -> products " << product1 << ", " << product2 << std::endl;
                reactionCount++;
            }
            
            // Find the specific reaction that occurred to get its type
            bool specialReactionHandled = false;
            for (const auto& reaction : mat->GetReactions()) {
                if (reaction.reactant == neighborMaterial) {
                    // Handle special reaction types that don't change materials normally
                    if (reaction.type == ReactionType::Electrify) {
                        // Apply electrified effect to the neighbor instead of changing material
                        uint8_t intensity = static_cast<uint8_t>(reaction.speed * 20); // Convert speed to intensity
                        uint8_t duration = static_cast<uint8_t>(reaction.range * 30);  // Convert range to duration
                        m_world->SetEffect(nx, ny, EffectLayer::Electrified, intensity, duration);
                        
                        // Debug electrification
                        static int electrifyCount = 0;
                        if (electrifyCount < 5) {
                            std::cout << "DEBUG: Electrify reaction applied! " << mat->GetName() 
                                     << " -> " << materialSystem->GetMaterialPtr(neighborMaterial)->GetName()
                                     << " intensity=" << (int)intensity << " duration=" << (int)duration << std::endl;
                            electrifyCount++;
                        }
                        
                        // For Electrify reactions, we don't change materials - just apply the effect
                        specialReactionHandled = true;
                        break;
                    }
                    
                    // Handle burning reactions - special case for fire + wood
                    if (reaction.type == ReactionType::Growth && 
                        mat->GetName() == "Fire" && 
                        materialSystem->GetMaterialPtr(neighborMaterial) && 
                        materialSystem->GetMaterialPtr(neighborMaterial)->GetName() == "Wood") {
                        
                        // Don't change wood to fire - add burning effect layer
                        uint8_t burnIntensity = static_cast<uint8_t>(reaction.speed * 150);
                        uint8_t burnDuration = static_cast<uint8_t>(reaction.range * 60);
                        m_world->SetEffect(nx, ny, EffectLayer::Burning, burnIntensity, burnDuration);
                        
                        // Sometimes create fire nearby or smoke
                        if (RandomChance(reaction.probability * 0.3f)) {
                            // Create fire in adjacent empty space
                            for (const auto& fireOffset : NEIGHBOR_OFFSETS) {
                                int fx = nx + fireOffset.first;
                                int fy = ny + fireOffset.second;
                                if (m_world->IsValidPosition(fx, fy) && 
                                    m_world->GetMaterial(fx, fy) == MATERIAL_EMPTY) {
                                    m_world->SetNextMaterial(fx, fy, currentMaterial); // Spread fire
                                    break;
                                }
                            }
                        }
                        
                        // Rare chance to emit smoke while burning
                        if (RandomChance(0.01f)) {
                            MaterialID smokeID = materialSystem->GetMaterialID("Smoke");
                            if (smokeID != MATERIAL_EMPTY) {
                                // Find empty space above for smoke
                                for (int sy = ny - 3; sy <= ny - 1; ++sy) {
                                    if (m_world->IsValidPosition(nx, sy) && 
                                        m_world->GetMaterial(nx, sy) == MATERIAL_EMPTY) {
                                        m_world->SetNextMaterial(nx, sy, smokeID);
                                        break;
                                    }
                                }
                            }
                        }
                        
                        // Extremely rare chance to create ash when wood burns (0.01% = 1 in 10,000)
                        if (RandomChance(0.0001f)) {
                            MaterialID ashID = materialSystem->GetMaterialID("Ash");
                            if (ashID != MATERIAL_EMPTY) {
                                // Find empty space nearby for ash
                                for (const auto& ashOffset : NEIGHBOR_OFFSETS) {
                                    int ax = nx + ashOffset.first;
                                    int ay = ny + ashOffset.second;
                                    if (m_world->IsValidPosition(ax, ay) && 
                                        m_world->GetMaterial(ax, ay) == MATERIAL_EMPTY) {
                                        m_world->SetNextMaterial(ax, ay, ashID);
                                        break;
                                    }
                                }
                            }
                        }
                        
                        // Fire + wood burning is a special reaction that doesn't change materials
                        specialReactionHandled = true;
                        break;
                    }
                    
                    // Handle explosive reactions
                    if (reaction.type == ReactionType::Explosive) {
                        float explosionPower = reaction.speed * reaction.probability;
                        float explosionRadius = static_cast<float>(reaction.range);
                        CreateExplosion(x, y, explosionPower, explosionRadius);
                        
                        // Explosive reactions still change materials after explosion
                        break;
                    }
                    
                    if (reaction.particleEffect) {
                        // TODO: Trigger particle effect at position (x, y)
                        // This would integrate with the ParticleSystem
                    }
                    
                    break; // Found the reaction, stop looking
                }
            }
            
            // Only apply material changes if it wasn't a special reaction that handles itself
            if (!specialReactionHandled) {
                // Set current cell to product1 in next grid
                m_world->SetNextMaterial(x, y, product1);
                
                // Set neighbor to product2 (if not empty) in next grid
                if (product2 != MATERIAL_EMPTY) {
                    m_world->SetNextMaterial(nx, ny, product2);
                }
            }
            
            // Only process one reaction per cell per frame to avoid cascading
            return;
        }
    }
}

void CellularAutomata::Update(float deltaTime) {
    if (!m_world) return;
    
    uint32_t width = m_world->GetWidth();
    uint32_t height = m_world->GetHeight();
    
    // CHECKERBOARD PATTERN: Process in two phases to prevent double-processing
    // This ensures each cell is only processed once per frame and prevents conflicts
    
    // Phase 1: Process "black" squares (x + y is even)
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            if ((x + y) % 2 == 0) {
                ProcessCell(x, y, deltaTime);
            }
        }
    }
    
    // Phase 2: Process "white" squares (x + y is odd)
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            if ((x + y) % 2 == 1) {
                ProcessCell(x, y, deltaTime);
            }
        }
    }
}

void CellularAutomata::ProcessCell(int x, int y, float deltaTime) {
    (void)deltaTime;
    const Cell& cell = m_world->GetCell(x, y);
    if (cell.material == MATERIAL_EMPTY) {
        return;
    }
    
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* material = materials->GetMaterialPtr(cell.material);
    if (!material) {
        return;
    }
    
    MaterialBehavior behavior = material->GetBehavior();
    
    switch (behavior) {
        case MaterialBehavior::Powder:
            ProcessPowder(x, y);
            break;
        case MaterialBehavior::Liquid:
            ProcessLiquid(x, y);
            break;
        case MaterialBehavior::Gas:
            ProcessGas(x, y);
            break;
        case MaterialBehavior::Fire:
            ProcessFire(x, y);
            break;
        case MaterialBehavior::Static:
            // Static materials don't move
            break;
    }
}

void CellularAutomata::ProcessPowder(int x, int y) {
    const Cell& cell = m_world->GetCell(x, y);
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* material = materials->GetMaterialPtr(cell.material);
    
    if (!material) return;
    
    float density = material->GetPhysicalProps().density;
    std::string materialName = material->GetName();
    
    // Get powder-specific properties (could be extended with more properties)
    float angleOfRepose = GetPowderAngleOfRepose(materialName);
    float cohesion = GetPowderCohesion(materialName);
    
    // Check if powder is stable (has solid support below and isn't on a steep slope)
    bool isStable = IsPowderStable(x, y);
    
    // Stable powders should move much less frequently to reduce jitter
    if (isStable && !RandomChance(0.05f)) { // Only 5% chance to move when stable
        return;
    }
    
    // 1. GRAVITY: Try to fall straight down first
    if (TryPowderMove(x, y, x, y + 1, density)) {
        return;
    }
    
    // 2. DIAGONAL FALLING: Try both diagonal directions with density check
    int fallDirection = RandomDirection();
    if (fallDirection != 0) {
        if (TryPowderMove(x, y, x + fallDirection, y + 1, density)) {
            return;
        }
        if (TryPowderMove(x, y, x - fallDirection, y + 1, density)) {
            return;
        }
    } else {
        // Try both directions if no preference
        if (TryPowderMove(x, y, x - 1, y + 1, density)) {
            return;
        }
        if (TryPowderMove(x, y, x + 1, y + 1, density)) {
            return;
        }
    }
    
    // 3. AVALANCHE BEHAVIOR: Angle of repose sliding (reduce frequency for stable powders)
    if (ShouldPowderSlide(x, y, angleOfRepose)) {
        int slideDirection = GetSlideDirection(x, y);
        if (slideDirection != 0 && TryPowderMove(x, y, x + slideDirection, y, density)) {
            return;
        }
    }
    
    // 4. COHESION EFFECTS: Some powders clump together
    if (cohesion > 0.5f && RandomChance(cohesion * 0.1f)) {
        // High cohesion powders resist movement
        return;
    }
}

void CellularAutomata::ProcessLiquid(int x, int y) {
    const Cell& cell = m_world->GetCell(x, y);
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* material = materials->GetMaterialPtr(cell.material);
    
    if (!material) return;
    
    // Get material-specific properties
    float viscosity = material->GetPhysicalProps().viscosity;
    float density = material->GetPhysicalProps().density;
    std::string materialName = material->GetName();
    
    // State transitions removed - using direct reactions instead
    
    // Material-specific liquid behaviors
    if (materialName == "Water") {
        ProcessWater(x, y, viscosity, density);
    } else if (materialName == "Oil") {
        ProcessOil(x, y, viscosity, density);
    } else if (materialName == "PoisonWater") {
        ProcessPoisonWater(x, y, viscosity, density);
    } else if (materialName == "LiquidNitrogen") {
        ProcessLiquidNitrogen(x, y, viscosity, density);
    } else if (materialName == "Lava") {
        ProcessLava(x, y, viscosity, density);
    } else if (materialName == "Acid") {
        ProcessAcid(x, y, viscosity, density);
    } else if (materialName == "Blood") {
        ProcessBlood(x, y, viscosity, density);
    } else if (materialName == "Quicksilver") {
        ProcessQuicksilver(x, y, viscosity, density);
    } else {
        // Generic liquid behavior
        ProcessGenericLiquid(x, y, viscosity, density);
    }
}

void CellularAutomata::ProcessWater(int x, int y, float viscosity, float density) {
    (void)viscosity; (void)density; // Parameters available for future use
    
    // 1. GRAVITY: Always try to fall straight down first (deterministic)
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    // 2. DIAGONAL FALLING: Try both diagonals with priority for less crowded areas
    if (TryMove(x, y, x - 1, y + 1)) {
        return;
    }
    if (TryMove(x, y, x + 1, y + 1)) {
        return;
    }
    
    // 3. HORIZONTAL FLOW: Careful implementation to prevent duplication
    // Only flow horizontally if we can't fall down anymore
    // This simulates real water behavior better
    
    // Check if there's solid ground or water below (can't fall)
    bool blockedBelow = false;
    if (m_world->IsValidPosition(x, y + 1)) {
        const Cell& belowCell = m_world->GetCell(x, y + 1);
        if (belowCell.material != MATERIAL_EMPTY) {
            blockedBelow = true;
        }
    } else {
        blockedBelow = true; // At bottom of world
    }
    
    // Only flow horizontally if we're resting on something
    if (blockedBelow) {
        // Random direction to prevent bias
        if (RandomChance(0.5f)) {
            if (TryMove(x, y, x - 1, y)) return;
            if (TryMove(x, y, x + 1, y)) return;
        } else {
            if (TryMove(x, y, x + 1, y)) return;
            if (TryMove(x, y, x - 1, y)) return;
        }
    }
    
    // 4. PRESSURE FLOW: Disabled to prevent mass conservation issues
    // The pressure equalization was causing water duplication
    // Water should only flow to immediately adjacent empty spaces
    // Long-range pressure effects are not realistic for cellular automata
}

void CellularAutomata::ProcessOil(int x, int y, float viscosity, float density) {
    (void)density; // Parameter available for future use
    
    // 1. BUOYANCY: Oil floats on denser liquids - check cell directly below
    if (m_world->IsValidPosition(x, y + 1)) {
        const Cell& belowCell = m_world->GetCell(x, y + 1);
        if (belowCell.material != MATERIAL_EMPTY) {
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* belowMat = materials->GetMaterialPtr(belowCell.material);
            
            if (belowMat && belowMat->GetBehavior() == MaterialBehavior::Liquid) {
                float belowDensity = belowMat->GetPhysicalProps().density;
                // FIXED: Use density comparison instead of hardcoded material name
                if (density < belowDensity - 0.05f) { // Oil is lighter, should float
                    // Oil should float - don't fall, try to rise if possible
                    if (TryMove(x, y, x, y - 1)) return; // Try to move up
                    if (TryMove(x, y, x - 1, y)) return; // Flow horizontally instead
                    if (TryMove(x, y, x + 1, y)) return;
                    return; // Don't fall through denser liquid
                }
            }
        }
    }
    
    // 2. GRAVITY: Oil falls but slower due to viscosity
    int fallChance = (int)(70 * (1.0f - viscosity * 0.3f)); // Viscosity reduces fall chance
    if (rand() % 100 < fallChance) {
        if (TryMove(x, y, x, y + 1)) return;
        if (TryMove(x, y, x - 1, y + 1)) return;
        if (TryMove(x, y, x + 1, y + 1)) return;
    }
    
    // 3. HORIZONTAL FLOW: Slower than water due to viscosity
    int flowChance = (int)(40 * (1.0f - viscosity * 0.5f));
    if (rand() % 100 < flowChance) {
        if (TryMove(x, y, x - 1, y)) return;
        if (TryMove(x, y, x + 1, y)) return;
    }
}

void CellularAutomata::ProcessPoisonWater(int x, int y, float viscosity, float density) {
    (void)viscosity; (void)density; // Parameters available for future use
    // Poison water behaves like water but spreads contamination
    
    // First, try to contaminate nearby water
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            const Cell& nextNeighborCell = m_world->GetNextCell(nx, ny);
            
            if (neighborCell.material != MATERIAL_EMPTY && 
                nextNeighborCell.material == neighborCell.material) {
                
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                
                if (neighborMat && neighborMat->GetName() == "Water") {
                    // Contaminate water with low probability
                    if (rand() % 100 < 5) { // 5% chance per frame
                        MaterialID poisonWaterID = materials->GetMaterialID("PoisonWater");
                        if (poisonWaterID != MATERIAL_EMPTY) {
                            m_world->SetNextMaterial(nx, ny, poisonWaterID);
                            m_world->SetNextTemperature(nx, ny, neighborCell.temperature);
                        }
                    }
                }
            }
        }
    }
    
    // FIXED: Don't double-process by calling ProcessWater
    // Instead, implement water-like behavior directly to avoid duplication
    
    // 1. GRAVITY: Try to fall straight down first
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    // 2. DIAGONAL FALLING: Try both diagonals  
    if (TryMove(x, y, x - 1, y + 1)) {
        return;
    }
    if (TryMove(x, y, x + 1, y + 1)) {
        return;
    }
    
    // 3. HORIZONTAL FLOW: Try one direction at a time
    if (TryMove(x, y, x - 1, y)) {
        return;
    }
    if (TryMove(x, y, x + 1, y)) {
        return;
    }
}

void CellularAutomata::ProcessGenericLiquid(int x, int y, float viscosity, float density) {
    (void)density; // Parameter available for future use
    
    // 1. GRAVITY: Always try to fall straight down first
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    // 2. DIAGONAL FALLING: Try both diagonals (viscosity affects chance)
    int fallChance = (int)(90 * (1.0f - viscosity * 0.5f)); // Higher viscosity = lower chance
    if (rand() % 100 < fallChance) {
        if (TryMove(x, y, x - 1, y + 1)) {
            return;
        }
        if (TryMove(x, y, x + 1, y + 1)) {
            return;
        }
    }
    
    // 3. HORIZONTAL FLOW: Direct flow to empty adjacent spaces
    int flowChance = (int)(70 * (1.0f - viscosity)); // Viscosity directly affects flow
    if (rand() % 100 < flowChance) {
        // Simple left/right flow
        if (TryMove(x, y, x - 1, y)) {
            return;
        }
        if (TryMove(x, y, x + 1, y)) {
            return;
        }
    }
}

void CellularAutomata::ProcessGas(int x, int y) {
    // Get gas properties for behavior
    const Cell& cell = m_world->GetCell(x, y);
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* material = materials->GetMaterialPtr(cell.material);
    
    if (!material) return;
    
    std::string name = material->GetName();
    float density = material->GetPhysicalProps().density;
    
    // Material-specific gas behaviors
    if (name == "Nitrogen") {
        ProcessNitrogen(x, y);
        return;
    } else if (name == "Steam") {
        ProcessSteam(x, y);
        return;
    } else if (name == "Smoke") {
        ProcessSmoke(x, y);
        return;
    } else if (name == "ToxicGas") {
        ProcessToxicGas(x, y);
        return;
    } else if (name == "CarbonDioxide") {
        ProcessCarbonDioxide(x, y);
        return;
    } else if (name == "Oxygen") {
        ProcessOxygen(x, y);
        return;
    } else if (name == "Hydrogen") {
        ProcessHydrogen(x, y);
        return;
    } else if (name == "Methane") {
        ProcessMethane(x, y);
        return;
    } else if (name == "Chlorine") {
        ProcessChlorine(x, y);
        return;
    } else if (name == "Ammonia") {
        ProcessAmmonia(x, y);
        return;
    } else if (name == "Helium") {
        ProcessHelium(x, y);
        return;
    } else if (name == "Argon") {
        ProcessArgon(x, y);
        return;
    } else if (name == "Neon") {
        ProcessNeon(x, y);
        return;
    } else if (name == "Propane") {
        ProcessPropane(x, y);
        return;
    } else if (name == "Acetylene") {
        ProcessAcetylene(x, y);
        return;
    } else if (name == "SulfurDioxide") {
        ProcessSulfurDioxide(x, y);
        return;
    } else if (name == "CarbonMonoxide") {
        ProcessCarbonMonoxide(x, y);
        return;
    } else if (name == "NitrousOxide") {
        ProcessNitrousOxide(x, y);
        return;
    } else if (name == "Ozone") {
        ProcessOzone(x, y);
        return;
    } else if (name == "Fluorine") {
        ProcessFluorine(x, y);
        return;
    } else if (name == "Xenon") {
        ProcessXenon(x, y);
        return;
    }
    
    // Generic gas behavior for unlisted gases
    ProcessGenericGas(x, y, density);
}

void CellularAutomata::ProcessFire(int x, int y) {
    // Simplified fire system - fires burn out quickly and spread to combustibles
    const Cell& cell = m_world->GetCell(x, y);
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* material = materials->GetMaterialPtr(cell.material);
    
    if (!material) return;
    
    // Special handling for Lightning
    if (material->GetName() == "Lightning") {
        ProcessLightning(x, y);
        return;
    }
    
    // Fire movement: rises due to buoyancy
    if (RandomChance(0.7f)) { // 70% chance to rise
        if (TryMove(x, y, x, y - 1)) return;
        
        // Try diagonal rise
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y - 1)) return;
    }
    
    // Slight horizontal drift
    if (RandomChance(0.1f)) {
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y)) return;
    }
    
    // Process normal fire behavior
    ProcessNormalFire(x, y, 60); // Fixed lifetime for now
    
    // Fire burns out quickly - no ash, just disappears
    if (RandomChance(0.05f)) { // 5% chance per frame to burn out
        m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
    }
}

void CellularAutomata::ProcessNormalFire(int x, int y, uint8_t fireLife) {
    (void)fireLife; // Available for intensity-based effects
    
    // Normal fire: Burns combustible materials directly
    MaterialSystem* materials = m_world->GetMaterialSystem();
    MaterialID woodID = materials->GetMaterialID("Wood");
    MaterialID fireID = materials->GetMaterialID("Fire");
    MaterialID waterID = materials->GetMaterialID("Water");
    MaterialID steamID = materials->GetMaterialID("Steam");
    
    // Check neighbors for burning and reactions
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            const Cell& nextNeighborCell = m_world->GetNextCell(nx, ny);
            
            // Only process if neighbor hasn't been modified
            if (neighborCell.material != MATERIAL_EMPTY && 
                nextNeighborCell.material == neighborCell.material) {
                
                // Direct reactions without temperature
                
                // Fire + Wood → Fire (spreading)
                if (neighborCell.material == woodID) {
                    if (RandomChance(0.02f)) { // 2% chance per frame
                        m_world->SetNextMaterial(nx, ny, fireID);
                    }
                }
                
                // Fire + Water → Steam (fire dies)
                if (neighborCell.material == waterID && steamID != MATERIAL_EMPTY) {
                    if (RandomChance(0.3f)) { // 30% chance per frame
                        m_world->SetNextMaterial(nx, ny, steamID);
                        m_world->SetNextMaterial(x, y, MATERIAL_EMPTY); // Fire dies
                        return;
                    }
                }
            }
        }
    }
    
    // Fire doesn't create ash - only wood burning creates ash
}

void CellularAutomata::ProcessLightning(int x, int y) {
    // Lightning creates branching electrical patterns and dies out quickly
    MaterialSystem* materials = m_world->GetMaterialSystem();
    MaterialID lightningID = materials->GetMaterialID("Lightning");
    
    // Lightning lasts very briefly (faster than fire)
    if (RandomChance(0.15f)) { // 15% chance per frame to disappear
        m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        return;
    }
    
    // Lightning creates branching patterns - tries to spread in multiple directions
    if (RandomChance(0.2f)) { // 20% chance to branch (much lower)
        // Create 1-2 lightning branches in different directions
        int branchCount = rand() % 2 + 1; // 1-2 branches only
        
        static const std::array<std::pair<int, int>, 8> directions = {{
            {0, -1}, {1, -1}, {1, 0}, {1, 1},   // Up, UpRight, Right, DownRight
            {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}  // Down, DownLeft, Left, UpLeft
        }};
        
        for (int i = 0; i < branchCount; ++i) {
            // Random direction for each branch
            int dirIndex = rand() % 8;
            int dx = directions[dirIndex].first;
            int dy = directions[dirIndex].second;
            
            // Try to create lightning line in this direction
            for (int step = 1; step <= 2; ++step) { // Up to 2 pixels in line only
                int nx = x + dx * step;
                int ny = y + dy * step;
                
                if (!m_world->IsValidPosition(nx, ny)) break;
                
                MaterialID neighborMaterial = m_world->GetMaterial(nx, ny);
                
                // Lightning spreads through empty space and conducts through metal/water
                if (neighborMaterial == MATERIAL_EMPTY) {
                    if (RandomChance(0.3f - step * 0.15f)) { // Much lower probability, decreases with distance
                        m_world->SetNextMaterial(nx, ny, lightningID);
                    }
                } else {
                    // Hit a material - check if it conducts
                    const Material* mat = materials->GetMaterialPtr(neighborMaterial);
                    if (mat && mat->GetName() == "Metal") {
                        // Metal conducts - continue lightning through it
                        if (RandomChance(0.9f)) {
                            m_world->SetNextMaterial(nx, ny, lightningID);
                        }
                    } else if (mat && mat->GetName() == "Water") {
                        // Water conducts but electrifies instead of becoming lightning
                        m_world->SetEffect(nx, ny, EffectLayer::Electrified, 255, 120);
                    } else {
                        // Non-conductive material - lightning stops
                        break;
                    }
                }
            }
        }
    }
    
    // Lightning doesn't move like fire - it stays in place but creates effects
    // Add electrical effect to current position
    m_world->SetEffect(x, y, EffectLayer::Electrified, 255, 60);
}

void CellularAutomata::ProcessLava(int x, int y, float viscosity, float density) {
    (void)density; // Parameter available for future use
    
    // Lava reactions with neighboring materials
    MaterialSystem* materials = m_world->GetMaterialSystem();
    
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            const Cell& nextNeighborCell = m_world->GetNextCell(nx, ny);
            
            if (neighborCell.material != MATERIAL_EMPTY && 
                nextNeighborCell.material == neighborCell.material) {
                
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                
                if (neighborMat) {
                    std::string neighborName = neighborMat->GetName();
                    
                    // Lava + Water/PoisonWater → Steam (with chance to harden lava)
                    if (neighborName == "Water" || neighborName == "PoisonWater") {
                        MaterialID steamID = materials->GetMaterialID("Steam");
                        if (steamID != MATERIAL_EMPTY) {
                            m_world->SetNextMaterial(nx, ny, steamID);
                            
                            // 15% chance lava hardens into stone
                            if (RandomChance(0.15f)) {
                                MaterialID stoneID = materials->GetMaterialID("Stone");
                                if (stoneID != MATERIAL_EMPTY) {
                                    m_world->SetNextMaterial(x, y, stoneID);
                                }
                            }
                        }
                    }
                    
                    // Lava + Wood/Oil → Fire
                    else if (neighborName == "Wood" || neighborName == "Oil") {
                        MaterialID fireID = materials->GetMaterialID("Fire");
                        if (fireID != MATERIAL_EMPTY) {
                            m_world->SetNextMaterial(nx, ny, fireID);
                        }
                    }
                    
                    // Lava + Ice → Water
                    else if (neighborName == "Ice") {
                        MaterialID waterID = materials->GetMaterialID("Water");
                        if (waterID != MATERIAL_EMPTY) {
                            m_world->SetNextMaterial(nx, ny, waterID);
                        }
                    }
                }
            }
        }
    }
    
    // 2. LIQUID BEHAVIOR: Lava flows slowly due to high viscosity
    // Very slow flow due to high viscosity
    if (rand() % 100 < (100 - (int)(viscosity * 80))) { // Viscosity heavily affects flow
        if (TryMove(x, y, x, y + 1)) {
            return;
        }
        
        // Try diagonal falling
        int direction = (rand() % 2) * 2 - 1;
        if (TryMove(x, y, x + direction, y + 1)) {
            return;
        }
        if (TryMove(x, y, x - direction, y + 1)) {
            return;
        }
    }
    
    // Horizontal flow - very limited due to high viscosity
    int liquidHeight = GetLiquidColumn(x, y);
    int leftHeight = GetLiquidColumn(x - 1, y);
    int rightHeight = GetLiquidColumn(x + 1, y);
    
    // Only flows under significant pressure
    if (liquidHeight > 4 && rand() % 100 < 25) { // 25% chance, needs height > 4
        bool shouldFlowLeft = leftHeight < liquidHeight - 3; // Needs big pressure difference
        bool shouldFlowRight = rightHeight < liquidHeight - 3;
        
        if (shouldFlowLeft || shouldFlowRight) {
            if (shouldFlowLeft && shouldFlowRight) {
                if (leftHeight < rightHeight) {
                    if (TryMove(x, y, x - 1, y)) return;
                    if (TryMove(x, y, x + 1, y)) return;
                } else {
                    if (TryMove(x, y, x + 1, y)) return;
                    if (TryMove(x, y, x - 1, y)) return;
                }
            } else if (shouldFlowLeft) {
                if (TryMove(x, y, x - 1, y)) return;
            } else if (shouldFlowRight) {
                if (TryMove(x, y, x + 1, y)) return;
            }
        }
    }
}

void CellularAutomata::ProcessAcid(int x, int y, float viscosity, float density) {
    (void)viscosity; (void)density; // Parameters available for future use
    
    // Acid corrodes materials it touches
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            const Cell& nextNeighborCell = m_world->GetNextCell(nx, ny);
            
            if (neighborCell.material != MATERIAL_EMPTY && 
                nextNeighborCell.material == neighborCell.material) {
                
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                
                if (neighborMat) {
                    std::string neighborName = neighborMat->GetName();
                    
                    // Violent reaction with water
                    if (neighborName == "Water") {
                        MaterialID toxicGasID = materials->GetMaterialID("ToxicGas");
                        if (toxicGasID != MATERIAL_EMPTY && rand() % 100 < 50) { // 50% chance
                            m_world->SetNextMaterial(nx, ny, toxicGasID);
                            m_world->SetNextTemperature(nx, ny, 80.0f);
                            // Acid is consumed in the reaction
                            if (rand() % 100 < 30) { // 30% chance acid is consumed
                                m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
                                return;
                            }
                        }
                    }
                    
                    // Corrode most solid materials
                    else if (neighborMat->GetBehavior() == MaterialBehavior::Static ||
                             neighborMat->GetBehavior() == MaterialBehavior::Powder) {
                        
                        // Some materials are resistant
                        if (neighborName == "Obsidian" || neighborName == "Ice") {
                            continue; // Acid-resistant materials
                        }
                        
                        // Corrode the material
                        if (rand() % 100 < 20) { // 20% chance per frame
                            m_world->SetNextMaterial(nx, ny, MATERIAL_EMPTY);
                            
                            // Small chance for acid to be consumed
                            if (rand() % 100 < 5) { // 5% chance
                                m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Acid flows like a normal liquid but slightly more aggressive
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    int direction = (rand() % 2) * 2 - 1;
    if (TryMove(x, y, x + direction, y + 1)) {
        return;
    }
    if (TryMove(x, y, x - direction, y + 1)) {
        return;
    }
    
    // Aggressive horizontal flow
    int liquidHeight = GetLiquidColumn(x, y);
    int leftHeight = GetLiquidColumn(x - 1, y);
    int rightHeight = GetLiquidColumn(x + 1, y);
    
    if (rand() % 100 < 80) { // 80% flow chance
        bool shouldFlowLeft = leftHeight < liquidHeight - 1;
        bool shouldFlowRight = rightHeight < liquidHeight - 1;
        
        if (shouldFlowLeft || shouldFlowRight) {
            if (shouldFlowLeft && shouldFlowRight) {
                if (leftHeight < rightHeight) {
                    if (TryMove(x, y, x - 1, y)) return;
                    if (TryMove(x, y, x + 1, y)) return;
                } else {
                    if (TryMove(x, y, x + 1, y)) return;
                    if (TryMove(x, y, x - 1, y)) return;
                }
            } else if (shouldFlowLeft) {
                if (TryMove(x, y, x - 1, y)) return;
            } else if (shouldFlowRight) {
                if (TryMove(x, y, x + 1, y)) return;
            }
        }
    }
}

void CellularAutomata::ProcessBlood(int x, int y, float viscosity, float density) {
    (void)density; // Parameter available for future use
    
    // Blood coagulates when in contact with solid surfaces and stationary
    bool touchingSolid = false;
    bool hasSpace = false;
    
    // Check if blood is touching a solid surface
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) {
                touchingSolid = true; // Treat world boundaries as solid
                continue;
            }
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            if (neighborCell.material == MATERIAL_EMPTY) {
                hasSpace = true;
            } else {
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                if (neighborMat && neighborMat->GetBehavior() == MaterialBehavior::Static) {
                    touchingSolid = true;
                }
            }
        }
    }
    
    // Coagulation occurs when blood is touching solid surfaces and has limited movement
    if (touchingSolid && !hasSpace && rand() % 100 < 8) { // 8% chance when trapped against solids
        MaterialSystem* materials = m_world->GetMaterialSystem();
        MaterialID clotID = materials->GetMaterialID("Clot");
        if (clotID != MATERIAL_EMPTY) {
            m_world->SetNextMaterial(x, y, clotID);
            m_world->SetNextTemperature(x, y, m_world->GetCell(x, y).temperature);
            return;
        }
    }
    // Lower chance for general coagulation over time
    else if (rand() % 100 < 1) { // 1% chance for general coagulation
        MaterialSystem* materials = m_world->GetMaterialSystem();
        MaterialID clotID = materials->GetMaterialID("Clot");
        if (clotID != MATERIAL_EMPTY) {
            m_world->SetNextMaterial(x, y, clotID);
            m_world->SetNextTemperature(x, y, m_world->GetCell(x, y).temperature);
            return;
        }
    }
    
    // Blood flows like a viscous liquid
    if (rand() % 100 < (100 - (int)(viscosity * 60))) { // Viscosity affects flow
        if (TryMove(x, y, x, y + 1)) {
            return;
        }
        
        int direction = (rand() % 2) * 2 - 1;
        if (TryMove(x, y, x + direction, y + 1)) {
            return;
        }
        if (TryMove(x, y, x - direction, y + 1)) {
            return;
        }
    }
    
    // Horizontal flow - slower than water due to viscosity
    int liquidHeight = GetLiquidColumn(x, y);
    int leftHeight = GetLiquidColumn(x - 1, y);
    int rightHeight = GetLiquidColumn(x + 1, y);
    
    int flowChance = (int)(50 * (1.0f - viscosity * 0.5f)); // Viscosity reduces flow
    
    if (rand() % 100 < flowChance) {
        bool shouldFlowLeft = leftHeight < liquidHeight - 1;
        bool shouldFlowRight = rightHeight < liquidHeight - 1;
        
        if (shouldFlowLeft || shouldFlowRight) {
            if (shouldFlowLeft && shouldFlowRight) {
                if (leftHeight < rightHeight) {
                    if (TryMove(x, y, x - 1, y)) return;
                    if (TryMove(x, y, x + 1, y)) return;
                } else {
                    if (TryMove(x, y, x + 1, y)) return;
                    if (TryMove(x, y, x - 1, y)) return;
                }
            } else if (shouldFlowLeft) {
                if (TryMove(x, y, x - 1, y)) return;
            } else if (shouldFlowRight) {
                if (TryMove(x, y, x + 1, y)) return;
            }
        }
    }
}

void CellularAutomata::ProcessQuicksilver(int x, int y, float viscosity, float density) {
    (void)viscosity; (void)density; // Parameters available for future use
    
    // Quicksilver has extreme density - it should sink through almost everything
    // Its primary behavior is handled by the density system in TryMove
    
    // Flows very quickly due to low viscosity
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    int direction = (rand() % 2) * 2 - 1;
    if (TryMove(x, y, x + direction, y + 1)) {
        return;
    }
    if (TryMove(x, y, x - direction, y + 1)) {
        return;
    }
    
    // Excellent horizontal flow due to very low viscosity
    int liquidHeight = GetLiquidColumn(x, y);
    int leftHeight = GetLiquidColumn(x - 1, y);
    int rightHeight = GetLiquidColumn(x + 1, y);
    
    if (rand() % 100 < 95) { // 95% flow chance - very fluid
        bool shouldFlowLeft = leftHeight < liquidHeight - 1;
        bool shouldFlowRight = rightHeight < liquidHeight - 1;
        
        if (shouldFlowLeft || shouldFlowRight) {
            if (shouldFlowLeft && shouldFlowRight) {
                if (leftHeight < rightHeight) {
                    if (TryMove(x, y, x - 1, y)) return;
                    if (TryMove(x, y, x + 1, y)) return;
                } else {
                    if (TryMove(x, y, x + 1, y)) return;
                    if (TryMove(x, y, x - 1, y)) return;
                }
            } else if (shouldFlowLeft) {
                if (TryMove(x, y, x - 1, y)) return;
            } else if (shouldFlowRight) {
                if (TryMove(x, y, x + 1, y)) return;
            }
        }
    }
    
    // Additional rapid flow attempts due to extreme fluidity
    if (rand() % 100 < 50) {
        for (int i = 0; i < 2; ++i) {
            int testDir = (rand() % 2) * 2 - 1;
            if (TryMove(x, y, x + testDir, y)) return;
        }
    }
}

void CellularAutomata::ProcessLiquidNitrogen(int x, int y, float viscosity, float density) {
    (void)viscosity; (void)density; // Parameters available for future use
    
    // Liquid nitrogen is extremely cold and evaporates quickly
    // It also freezes nearby liquids
    
    // 1. COOLING EFFECT: Freeze nearby liquids
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            const Cell& nextNeighborCell = m_world->GetNextCell(nx, ny);
            
            if (neighborCell.material != MATERIAL_EMPTY && 
                nextNeighborCell.material == neighborCell.material) {
                
                // Drastically reduce temperature of nearby materials
                float currentTemp = neighborCell.temperature;
                float newTemp = currentTemp - 50.0f; // Extreme cooling
                m_world->SetNextTemperature(nx, ny, newTemp);
                
                // Try to freeze liquids
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                
                if (neighborMat && neighborMat->GetBehavior() == MaterialBehavior::Liquid) {
                    std::string neighborName = neighborMat->GetName();
                    
                    // Freeze water instantly when touched by liquid nitrogen
                    if (neighborName == "Water" && rand() % 100 < 80) { // 80% chance
                        MaterialID iceID = materials->GetMaterialID("Ice");
                        if (iceID != MATERIAL_EMPTY) {
                            m_world->SetNextMaterial(nx, ny, iceID);
                            m_world->SetNextTemperature(nx, ny, -50.0f);
                        }
                    }
                    
                    // Freeze poison water too
                    else if (neighborName == "PoisonWater" && rand() % 100 < 70) { // 70% chance
                        MaterialID iceID = materials->GetMaterialID("Ice");
                        if (iceID != MATERIAL_EMPTY) {
                            m_world->SetNextMaterial(nx, ny, iceID);
                            m_world->SetNextTemperature(nx, ny, -50.0f);
                        }
                    }
                }
                
                // Extinguish fire
                if (neighborMat && neighborMat->GetName() == "Fire") {
                    if (rand() % 100 < 95) { // 95% chance to extinguish
                        m_world->SetNextMaterial(nx, ny, MATERIAL_EMPTY);
                        m_world->SetNextTemperature(nx, ny, -100.0f);
                    }
                }
            }
        }
    }
    
    // 2. EVAPORATION: Liquid nitrogen boils at -196°C, evaporates quickly at room temp
    float currentTemp = m_world->GetCell(x, y).temperature;
    if (currentTemp > -196.0f) {
        // Chance to evaporate increases with temperature
        int evaporationChance = (int)((currentTemp + 196.0f) * 2); // 0% at -196°C, 40% at 20°C
        evaporationChance = std::min(90, evaporationChance); // Cap at 90%
        
        if (rand() % 100 < evaporationChance) {
            MaterialSystem* materials = m_world->GetMaterialSystem();
            MaterialID nitrogenGasID = materials->GetMaterialID("Nitrogen");
            if (nitrogenGasID != MATERIAL_EMPTY) {
                m_world->SetNextMaterial(x, y, nitrogenGasID);
                m_world->SetNextTemperature(x, y, currentTemp + 10.0f); // Slightly warmer as gas
                return;
            }
        }
    }
    
    // 3. LIQUID BEHAVIOR: Flows like a very fluid liquid
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    // Try diagonal falling
    int direction = (rand() % 2) * 2 - 1;
    if (TryMove(x, y, x + direction, y + 1)) {
        return;
    }
    if (TryMove(x, y, x - direction, y + 1)) {
        return;
    }
    
    // Horizontal flow - very fluid due to low viscosity
    int liquidHeight = GetLiquidColumn(x, y);
    int leftHeight = GetLiquidColumn(x - 1, y);
    int rightHeight = GetLiquidColumn(x + 1, y);
    
    if (rand() % 100 < 85) { // High flow chance
        bool shouldFlowLeft = leftHeight < liquidHeight - 1;
        bool shouldFlowRight = rightHeight < liquidHeight - 1;
        
        if (shouldFlowLeft || shouldFlowRight) {
            if (shouldFlowLeft && shouldFlowRight) {
                if (leftHeight < rightHeight) {
                    if (TryMove(x, y, x - 1, y)) return;
                    if (TryMove(x, y, x + 1, y)) return;
                } else {
                    if (TryMove(x, y, x + 1, y)) return;
                    if (TryMove(x, y, x - 1, y)) return;
                }
            } else if (shouldFlowLeft) {
                if (TryMove(x, y, x - 1, y)) return;
            } else if (shouldFlowRight) {
                if (TryMove(x, y, x + 1, y)) return;
            }
        }
    }
}

void CellularAutomata::ProcessNitrogen(int x, int y) {
    // Nitrogen gas behavior: rises, spreads, and cools surroundings
    
    // 1. COOLING EFFECT: Nitrogen gas cools nearby materials (less than liquid)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            const Cell& nextNeighborCell = m_world->GetNextCell(nx, ny);
            
            if (neighborCell.material != MATERIAL_EMPTY && 
                nextNeighborCell.material == neighborCell.material) {
                
                // Moderate cooling effect
                float currentTemp = neighborCell.temperature;
                if (currentTemp > 0.0f) {
                    float newTemp = currentTemp - 5.0f; // Moderate cooling
                    m_world->SetNextTemperature(nx, ny, newTemp);
                }
                
                // Extinguish fire
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                
                if (neighborMat && neighborMat->GetName() == "Fire") {
                    if (rand() % 100 < 60) { // 60% chance to extinguish
                        m_world->SetNextMaterial(nx, ny, MATERIAL_EMPTY);
                        m_world->SetNextTemperature(nx, ny, 10.0f);
                    }
                }
            }
        }
    }
    
    // 2. CONDENSATION: If nitrogen gets very cold, it can become liquid
    float currentTemp = m_world->GetCell(x, y).temperature;
    if (currentTemp <= -196.0f) {
        if (rand() % 100 < 15) { // 15% chance to condense when very cold
            MaterialSystem* materials = m_world->GetMaterialSystem();
            MaterialID liquidNitrogenID = materials->GetMaterialID("LiquidNitrogen");
            if (liquidNitrogenID != MATERIAL_EMPTY) {
                m_world->SetNextMaterial(x, y, liquidNitrogenID);
                m_world->SetNextTemperature(x, y, currentTemp);
                return;
            }
        }
    }
    
    // 3. GAS MOVEMENT: Nitrogen spreads and rises like other gases
    // Try to rise up
    if (rand() % 100 < 70) { // 70% chance to rise
        if (TryMove(x, y, x, y - 1)) {
            return;
        }
        
        // Try diagonal rising
        int direction = (rand() % 2) * 2 - 1;
        if (TryMove(x, y, x + direction, y - 1)) {
            return;
        }
        if (TryMove(x, y, x - direction, y - 1)) {
            return;
        }
    }
    
    // Horizontal spreading
    if (rand() % 100 < 80) { // 80% chance to spread
        int direction = (rand() % 2) * 2 - 1;
        if (TryMove(x, y, x + direction, y)) {
            return;
        }
        if (TryMove(x, y, x - direction, y)) {
            return;
        }
    }
    
    // Random dispersal
    if (rand() % 100 < 30) {
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                if (rand() % 25 == 0) { // Random dispersal
                    if (TryMove(x, y, x + dx, y + dy)) {
                        return;
                    }
                }
            }
        }
    }
}

bool CellularAutomata::TryMove(int fromX, int fromY, int toX, int toY) {
    if (!m_world->IsValidPosition(toX, toY)) {
        return false;
    }
    
    // Read from current grid (source state)
    const Cell& fromCell = m_world->GetCell(fromX, fromY);
    const Cell& toCell = m_world->GetCell(toX, toY);
    
    // CRITICAL: Also check what's already in the destination in the NEXT grid
    // This prevents multiple materials from moving to the same spot
    const Cell& nextToCell = m_world->GetNextCell(toX, toY);
    
    // Can only move to empty space in BOTH current and next grids
    if (toCell.material == MATERIAL_EMPTY && nextToCell.material == MATERIAL_EMPTY) {
        // Move the particle to next grid
        m_world->SetNextMaterial(toX, toY, fromCell.material);
        m_world->SetNextMaterial(fromX, fromY, MATERIAL_EMPTY);
        
        // Copy other properties (temperature removed)
        
        return true;
    }
    
    // Handle density-based displacement for non-empty destinations
    // FIXED: Check that the destination hasn't been modified by another cell
    if (toCell.material != MATERIAL_EMPTY && nextToCell.material == toCell.material) {
        MaterialSystem* materials = m_world->GetMaterialSystem();
        const Material* fromMaterial = materials->GetMaterialPtr(fromCell.material);
        const Material* toMaterial = materials->GetMaterialPtr(toCell.material);
        
        if (fromMaterial && toMaterial) {
            float fromDensity = fromMaterial->GetPhysicalProps().density;
            float toDensity = toMaterial->GetPhysicalProps().density;
            
            MaterialBehavior fromBehavior = fromMaterial->GetBehavior();
            MaterialBehavior toBehavior = toMaterial->GetBehavior();
            
            // Static materials (like stone, wood) should never be displaced
            if (toBehavior == MaterialBehavior::Static) {
                return false; // Cannot move into static materials
            }
            
            // ENHANCED INTERACTION RULES:
            
            // RULE 1: Dense materials displace lighter materials (gravity-based)
            if (fromDensity > toDensity + 0.05f) { // Reduced threshold for more responsive movement
                // Powders fall through everything lighter (except static materials and other powders)
                if (fromBehavior == MaterialBehavior::Powder) {
                    if (toBehavior != MaterialBehavior::Static && toBehavior != MaterialBehavior::Powder) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                    // NO POWDER-POWDER DISPLACEMENT: Powders always stack on top of each other
                    // This is realistic - powders don't displace each other in real life
                }
                
                // Special case: Heavy liquids (Lava, Magma) displace gases and other liquids, but NOT powders
                if (fromBehavior == MaterialBehavior::Liquid && fromDensity > 2.5f) {
                    if (toBehavior == MaterialBehavior::Gas ||
                        (toBehavior == MaterialBehavior::Liquid && fromDensity > toDensity + 0.3f)) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                    // Liquids should NOT displace powders - they should land on top or react
                }
                
                // Heavy liquids displace lighter liquids and gases
                if (fromBehavior == MaterialBehavior::Liquid) {
                    if (toBehavior == MaterialBehavior::Gas || 
                        (toBehavior == MaterialBehavior::Liquid && fromDensity > toDensity + 0.1f) ||
                        toBehavior == MaterialBehavior::Fire) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                }
            }
            
            // RULE 2: Light materials rise THROUGH denser materials (buoyancy)
            if (fromDensity < toDensity - 0.05f) {
                // Gases rise through everything except static materials
                if (fromBehavior == MaterialBehavior::Gas) {
                    if (toBehavior == MaterialBehavior::Powder || toBehavior == MaterialBehavior::Liquid || toBehavior == MaterialBehavior::Fire) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                }
                
                // Fire rises through liquids and powders (flames are buoyant)
                if (fromBehavior == MaterialBehavior::Fire) {
                    if (toBehavior == MaterialBehavior::Powder || toBehavior == MaterialBehavior::Liquid) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                }
                
                // Light liquids rise through dense liquids (oil through water, water through lava)
                if (fromBehavior == MaterialBehavior::Liquid && toBehavior == MaterialBehavior::Liquid) {
                    if (fromDensity < toDensity - 0.15f) { // Optimized threshold for core materials
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                }
                
                // Ultra-light powders (Snow, Dust) float on most liquids
                if (fromBehavior == MaterialBehavior::Powder && fromDensity < 0.5f) {
                    if (toBehavior == MaterialBehavior::Liquid) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                }
            }
            
            // RULE 3: Liquid immiscibility and density separation
            if (fromBehavior == MaterialBehavior::Liquid && toBehavior == MaterialBehavior::Liquid) {
                std::string fromName = fromMaterial->GetName();
                std::string toName = toMaterial->GetName();
                
                // Water and Oil don't mix - they separate by density
                if ((fromName == "Water" && toName == "Oil") || 
                    (fromName == "Oil" && toName == "Water")) {
                    
                    // Only allow separation based on density, not mixing
                    if (fromDensity > toDensity + 0.05f) {
                        // Denser liquid (water) displaces lighter liquid (oil)
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                    // Otherwise, liquids don't mix and movement is blocked
                    return false;
                }
                
                // PoisonWater can mix with Water but slowly
                if ((fromName == "PoisonWater" && toName == "Water") ||
                    (fromName == "Water" && toName == "PoisonWater")) {
                    
                    // Allow mixing but prefer density separation
                    if (abs(fromDensity - toDensity) > 0.02f) {
                        if (fromDensity > toDensity + 0.02f) {
                            SwapCells(fromX, fromY, toX, toY);
                            return true;
                        }
                    } else {
                        // Similar density - allow slow mixing
                        if (rand() % 100 < 20) { // 20% chance to mix
                            SwapCells(fromX, fromY, toX, toY);
                            return true;
                        }
                    }
                    return false;
                }
                
                // Generic liquid-liquid interactions based on density
                if (fromDensity > toDensity + 0.1f) {
                    SwapCells(fromX, fromY, toX, toY);
                    return true;
                }
                
                // Liquids of similar density mix slowly
                if (abs(fromDensity - toDensity) < 0.05f && rand() % 100 < 30) {
                    SwapCells(fromX, fromY, toX, toY);
                    return true;
                }
                
                return false; // Block movement if no mixing conditions met
            }
            
            // RULE 4: Liquids landing on powders should NOT displace them
            if (fromBehavior == MaterialBehavior::Liquid && toBehavior == MaterialBehavior::Powder) {
                // Liquids should just land on top of powders, not displace through them
                // Displacement only happens when powder falls INTO liquid (handled in powder logic)
                return false;
            }
            
            // RULE 5: Powders falling into liquids (correct direction for displacement)
            if (fromBehavior == MaterialBehavior::Powder && toBehavior == MaterialBehavior::Liquid) {
                // Dense powder sinks when falling into lighter liquid (sand into water)
                // Light powder floats when falling into denser liquid (dust into molten metal)
                if (fromDensity > toDensity + 0.1f) {
                    SwapCells(fromX, fromY, toX, toY);
                    return true;
                }
                // Light powders float on dense liquids
                return false;
            }
        }
    }
    
    return false;
}

void CellularAutomata::SwapCells(int x1, int y1, int x2, int y2) {
    // Use proper double buffering - write to next grid
    if (!m_world->IsValidPosition(x1, y1) || !m_world->IsValidPosition(x2, y2)) {
        return;
    }
    
    // Get current cell data
    const Cell& cell1 = m_world->GetCell(x1, y1);
    const Cell& cell2 = m_world->GetCell(x2, y2);
    
    // Verify that neither cell has been modified by another process
    const Cell& nextCell1 = m_world->GetNextCell(x1, y1);
    const Cell& nextCell2 = m_world->GetNextCell(x2, y2);
    
    // Only swap if both cells are in their original state in the next grid
    if (nextCell1.material == cell1.material && nextCell2.material == cell2.material) {
        // Write swapped data to next grid
        m_world->SetNextMaterial(x1, y1, cell2.material);
        m_world->SetNextMaterial(x2, y2, cell1.material);
    }
    // If materials have been modified, the swap fails silently to preserve conservation
}

int CellularAutomata::GetPileHeight(int x, int y) const {
    // Count consecutive powder materials going up from this position
    int height = 0;
    for (int checkY = y; checkY >= 0; --checkY) {
        MaterialID mat = m_world->GetMaterial(x, checkY);
        if (mat == MATERIAL_EMPTY) break;
        
        MaterialSystem* materials = m_world->GetMaterialSystem();
        const Material* material = materials->GetMaterialPtr(mat);
        if (material && material->GetBehavior() == MaterialBehavior::Powder) {
            height++;
        } else {
            break;
        }
    }
    return height;
}

int CellularAutomata::GetLiquidColumn(int x, int y) const {
    // Count consecutive liquid materials going up from this position
    int height = 0;
    for (int checkY = y; checkY >= 0; --checkY) {
        MaterialID mat = m_world->GetMaterial(x, checkY);
        if (mat == MATERIAL_EMPTY) break;
        
        MaterialSystem* materials = m_world->GetMaterialSystem();
        const Material* material = materials->GetMaterialPtr(mat);
        if (material && material->GetBehavior() == MaterialBehavior::Liquid) {
            height++;
        } else {
            break;
        }
    }
    return height;
}

MaterialID CellularAutomata::GetWoodMaterialID() const {
    // Look up wood material by name
    MaterialSystem* materials = m_world->GetMaterialSystem();
    return materials->GetMaterialID("Wood");
}

MaterialID CellularAutomata::GetFireMaterialID() const {
    // Look up fire material by name
    MaterialSystem* materials = m_world->GetMaterialSystem();
    return materials->GetMaterialID("Fire");
}

// ===== GAS SYSTEM PROCESSORS =====

void CellularAutomata::ProcessSteam(int x, int y) {
    // Steam rises aggressively with strong turbulence
    if (TryTurbulentMovement(x, y, 0.7f, 0.8f, 4)) return;
    
    // Steam occasionally condenses back to water
    if (RandomChance(0.01f)) { // 1% chance per frame
        MaterialSystem* materials = m_world->GetMaterialSystem();
        MaterialID waterID = materials->GetMaterialID("Water");
        if (waterID != MATERIAL_EMPTY) {
            m_world->SetNextMaterial(x, y, waterID);
        }
    }
    
    // Steam occasionally dissipates completely
    else if (RandomChance(0.005f)) { // 0.5% chance per frame
        m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
    }
}

void CellularAutomata::ProcessSmoke(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Smoke has chaotic swirling movement with moderate upward bias
    if (TryTurbulentMovement(x, y, 0.5f, 0.9f, 5)) return;
    
    // Smoke dissipates over time using the life field
    if (cell.life == 0) {
        cell.life = 255; // Initialize life when first created
    } else {
        cell.life--;
        if (cell.life < 50 && RandomChance(0.02f)) {
            // Smoke dissipates to empty space
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessToxicGas(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Toxic gas spreads chaotically with low upward bias (dense gas)
    if (TryTurbulentMovement(x, y, 0.2f, 0.9f, 4)) return;
    
    // Toxic gas has longer lifetime than smoke but still dissipates
    if (cell.life == 0) {
        cell.life = 200; // Longer initial life
    } else {
        cell.life--;
        if (cell.life < 30 && RandomChance(0.01f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
    
    // Toxic gas contaminates nearby materials
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            if (neighborCell.material != MATERIAL_EMPTY && RandomChance(0.01f)) {
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
                
                if (neighborMat && neighborMat->GetName() == "Water") {
                    // Convert water to poison water
                    MaterialID poisonWaterID = materials->GetMaterialID("PoisonWater");
                    if (poisonWaterID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, poisonWaterID);
                    }
                }
            }
        }
    }
}

void CellularAutomata::ProcessCarbonDioxide(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // CO2 is denser than air, tends to sink and accumulate
    if (RandomChance(0.4f) && TryMove(x, y, x, y + 1)) return;
    if (RandomChance(0.3f) && TryMove(x, y, x - 1, y + 1)) return;
    if (RandomChance(0.3f) && TryMove(x, y, x + 1, y + 1)) return;
    
    // Moderate horizontal dispersion
    if (RandomChance(0.3f)) {
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y)) return;
    }
    
    // CO2 can freeze into dry ice at very low temperatures
    float temperature = m_world->GetTemperature(x, y);
    if (temperature < -78.0f && RandomChance(0.05f)) {
        MaterialSystem* materials = m_world->GetMaterialSystem();
        MaterialID dryIceID = materials->GetMaterialID("DryIce");
        if (dryIceID != MATERIAL_EMPTY) {
            m_world->SetNextMaterial(x, y, dryIceID);
            m_world->SetNextTemperature(x, y, -78.0f);
        }
    }
    
    // CO2 dissipates very slowly
    if (cell.life == 0) {
        cell.life = 255;
    } else if (RandomChance(0.001f)) {
        cell.life--;
        if (cell.life < 10) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessOxygen(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Oxygen is lighter than CO2 but heavier than hydrogen
    if (RandomChance(0.5f) && TryMove(x, y, x, y - 1)) return;
    
    // Good dispersion - oxygen spreads evenly
    if (RandomChance(0.5f)) {
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y)) return;
    }
    
    // Oxygen feeds fire - increase fire spread rate nearby
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire") {
                // Increase fire temperature when near oxygen
                float currentTemp = m_world->GetTemperature(nx, ny);
                m_world->SetNextTemperature(nx, ny, currentTemp + 10.0f);
            }
        }
    }
    
    // Oxygen persists longer than most gases
    if (cell.life == 0) {
        cell.life = 255;
    } else if (RandomChance(0.0005f)) {
        cell.life--;
        if (cell.life < 5) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessHydrogen(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Hydrogen rises rapidly with high turbulence
    if (TryTurbulentMovement(x, y, 0.8f, 0.9f, 5)) return;
    
    // Hydrogen is highly flammable - explodes near fire
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire" && RandomChance(0.1f)) {
                // Hydrogen explodes - creates fire in a small radius
                for (int ey = -1; ey <= 1; ++ey) {
                    for (int ex = -1; ex <= 1; ++ex) {
                        int explosionX = x + ex, explosionY = y + ey;
                        if (m_world->IsValidPosition(explosionX, explosionY)) {
                            MaterialID fireID = materials->GetMaterialID("Fire");
                            if (fireID != MATERIAL_EMPTY) {
                                m_world->SetNextMaterial(explosionX, explosionY, fireID);
                                m_world->SetNextTemperature(explosionX, explosionY, 1000.0f);
                            }
                        }
                    }
                }
                return; // Hydrogen consumed in explosion
            }
        }
    }
    
    // Hydrogen escapes quickly
    if (cell.life == 0) {
        cell.life = 150; // Shorter lifespan
    } else {
        cell.life--;
        if (cell.life < 50 && RandomChance(0.03f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessMethane(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Methane is lighter than air but not as light as hydrogen
    if (RandomChance(0.7f) && TryMove(x, y, x, y - 1)) return;
    if (RandomChance(0.5f) && TryMove(x, y, x - 1, y - 1)) return;
    if (RandomChance(0.5f) && TryMove(x, y, x + 1, y - 1)) return;
    
    // Good horizontal dispersion
    if (RandomChance(0.5f)) {
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y)) return;
    }
    
    // Methane burns when exposed to fire
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire" && RandomChance(0.05f)) {
                // Methane burns to CO2 and water vapor (steam)
                if (RandomChance(0.5f)) {
                    MaterialID co2ID = materials->GetMaterialID("CarbonDioxide");
                    if (co2ID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, co2ID);
                    }
                } else {
                    MaterialID steamID = materials->GetMaterialID("Steam");
                    if (steamID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, steamID);
                        m_world->SetNextTemperature(x, y, 150.0f);
                    }
                }
                return;
            }
        }
    }
    
    // Methane dissipates moderately
    if (cell.life == 0) {
        cell.life = 180;
    } else {
        cell.life--;
        if (cell.life < 30 && RandomChance(0.015f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessChlorine(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Chlorine is denser than air, tends to sink and pool
    if (RandomChance(0.6f) && TryMove(x, y, x, y + 1)) return;
    if (RandomChance(0.4f) && TryMove(x, y, x - 1, y + 1)) return;
    if (RandomChance(0.4f) && TryMove(x, y, x + 1, y + 1)) return;
    
    // Limited horizontal dispersion
    if (RandomChance(0.3f)) {
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y)) return;
    }
    
    // Chlorine is highly reactive and toxic
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && RandomChance(0.02f)) {
                // Chlorine reacts with water to form acid
                if (neighborMat->GetName() == "Water") {
                    MaterialID acidID = materials->GetMaterialID("Acid");
                    if (acidID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, acidID);
                    }
                }
                // Chlorine converts organic materials to toxic gas
                else if (neighborMat->GetName() == "Wood" || neighborMat->GetName() == "Blood") {
                    MaterialID toxicID = materials->GetMaterialID("ToxicGas");
                    if (toxicID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, toxicID);
                    }
                }
            }
        }
    }
    
    // Chlorine persists but gradually dissipates
    if (cell.life == 0) {
        cell.life = 200;
    } else {
        cell.life--;
        if (cell.life < 20 && RandomChance(0.008f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessAmmonia(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Ammonia is lighter than air, rises moderately
    if (RandomChance(0.6f) && TryMove(x, y, x, y - 1)) return;
    if (RandomChance(0.4f) && TryMove(x, y, x - 1, y - 1)) return;
    if (RandomChance(0.4f) && TryMove(x, y, x + 1, y - 1)) return;
    
    // Good horizontal dispersion
    if (RandomChance(0.4f)) {
        int dir = RandomDirection();
        if (dir != 0 && TryMove(x, y, x + dir, y)) return;
    }
    
    // Ammonia can freeze into liquid at very low temperatures
    float temperature = m_world->GetTemperature(x, y);
    if (temperature < -33.0f && RandomChance(0.03f)) {
        MaterialSystem* materials = m_world->GetMaterialSystem();
        MaterialID liquidAmmoniaID = materials->GetMaterialID("LiquidAmmonia");
        if (liquidAmmoniaID != MATERIAL_EMPTY) {
            m_world->SetNextMaterial(x, y, liquidAmmoniaID);
            m_world->SetNextTemperature(x, y, -33.0f);
        }
    }
    
    // Ammonia reacts with acids
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Acid" && RandomChance(0.1f)) {
                // Ammonia neutralizes acid, creating salt water
                MaterialID waterID = materials->GetMaterialID("Water");
                if (waterID != MATERIAL_EMPTY) {
                    m_world->SetNextMaterial(nx, ny, waterID);
                    m_world->SetNextMaterial(x, y, MATERIAL_EMPTY); // Ammonia consumed
                }
                return;
            }
        }
    }
    
    // Ammonia dissipates moderately
    if (cell.life == 0) {
        cell.life = 180;
    } else {
        cell.life--;
        if (cell.life < 40 && RandomChance(0.01f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessHelium(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Helium rises extremely fast with maximum turbulence
    if (TryTurbulentMovement(x, y, 0.9f, 1.0f, 6)) return;
    
    // Try rapid multi-cell rises for very light gas
    if (RandomChance(0.5f) && TryMove(x, y, x, y - 2)) return;
    
    // Helium is completely inert - no reactions
    
    // Helium escapes very quickly
    if (cell.life == 0) {
        cell.life = 100; // Very short lifespan
    } else {
        cell.life--;
        if (cell.life < 40 && RandomChance(0.05f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessGenericGas(int x, int y, float density) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Generic gas behavior based on density with turbulent movement
    float upwardBias = std::max(0.1f, std::min(0.9f, 1.0f - (density * 0.5f))); // Lower density = higher rise chance
    float turbulence = 0.8f; // All gases are turbulent
    int attempts = 4;
    
    if (TryTurbulentMovement(x, y, upwardBias, turbulence, attempts)) return;
    
    // Generic dissipation
    if (cell.life == 0) {
        cell.life = static_cast<uint8_t>(std::max(50.0f, 200.0f * (1.0f - density))); // Lighter gases dissipate faster
    } else {
        cell.life--;
        if (cell.life < 20 && RandomChance(0.01f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

bool CellularAutomata::TryTurbulentMovement(int x, int y, float upwardBias, float horizontalTurbulence, int attempts) {
    // Gas-like turbulent movement with configurable bias
    for (int attempt = 0; attempt < attempts; ++attempt) {
        int dx = RandomDirection();
        int dy = RandomDirection();
        
        // Apply upward bias
        if (RandomChance(upwardBias)) {
            dy = -1; // Rise
        } else if (RandomChance(0.3f)) {
            dy = 0;  // Stay level
        }
        // Otherwise keep random dy (could be down)
        
        // Apply horizontal turbulence
        if (RandomChance(horizontalTurbulence)) {
            dx = RandomDirection();
        }
        
        // Try the turbulent movement
        if (TryMove(x, y, x + dx, y + dy)) return true;
        
        // Also try some pure random movements for extra chaos
        if (RandomChance(0.3f)) {
            int random_dx = static_cast<int>((Random01() * 3.0f) - 1.0f); // -1, 0, or 1 with noise
            int random_dy = static_cast<int>((Random01() * 3.0f) - 1.0f);
            if (TryMove(x, y, x + random_dx, y + random_dy)) return true;
        }
    }
    return false;
}

// ===== NEW GAS PROCESSORS WITH INTERACTIONS =====

void CellularAutomata::ProcessArgon(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Argon is dense and inert - minimal upward movement
    if (TryTurbulentMovement(x, y, 0.1f, 0.6f, 3)) return;
    
    // Argon is completely inert - no reactions
    // But it displaces other gases due to high density
    
    // Simple lifecycle
    if (cell.life == 0) {
        cell.life = 255;
    } else if (RandomChance(0.0001f)) {
        cell.life--;
        if (cell.life < 5) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessNeon(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Neon is light but not as much as helium
    if (TryTurbulentMovement(x, y, 0.6f, 0.8f, 4)) return;
    
    // Neon glows when near electrical sources (fire represents energy)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire") {
                // Neon "glows" by increasing its temperature
                float currentTemp = m_world->GetTemperature(x, y);
                m_world->SetNextTemperature(x, y, currentTemp + 5.0f);
            }
        }
    }
    
    // Noble gas - escapes slowly
    if (cell.life == 0) {
        cell.life = 200;
    } else {
        cell.life--;
        if (cell.life < 30 && RandomChance(0.01f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessPropane(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Propane is heavy fuel gas - tends to pool
    if (TryTurbulentMovement(x, y, 0.2f, 0.7f, 3)) return;
    
    // Propane burns aggressively
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire" && RandomChance(0.15f)) {
                // Propane burns to CO2 and steam, creates more fire
                if (RandomChance(0.4f)) {
                    MaterialID co2ID = materials->GetMaterialID("CarbonDioxide");
                    if (co2ID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, co2ID);
                    }
                } else if (RandomChance(0.4f)) {
                    MaterialID steamID = materials->GetMaterialID("Steam");
                    if (steamID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, steamID);
                        m_world->SetNextTemperature(x, y, 200.0f);
                    }
                } else {
                    MaterialID fireID = materials->GetMaterialID("Fire");
                    if (fireID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, fireID);
                        m_world->SetNextTemperature(x, y, 900.0f);
                    }
                }
                return;
            }
        }
    }
    
    // Propane persists
    if (cell.life == 0) {
        cell.life = 255;
    } else {
        cell.life--;
        if (cell.life < 50 && RandomChance(0.005f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessAcetylene(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Acetylene rises moderately but is very explosive
    if (TryTurbulentMovement(x, y, 0.5f, 0.9f, 4)) return;
    
    // Acetylene is EXTREMELY explosive
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire" && RandomChance(0.2f)) {
                // Acetylene creates massive explosion
                for (int ey = -2; ey <= 2; ++ey) {
                    for (int ex = -2; ex <= 2; ++ex) {
                        int explosionX = x + ex, explosionY = y + ey;
                        if (m_world->IsValidPosition(explosionX, explosionY)) {
                            MaterialID fireID = materials->GetMaterialID("Fire");
                            if (fireID != MATERIAL_EMPTY) {
                                m_world->SetNextMaterial(explosionX, explosionY, fireID);
                                m_world->SetNextTemperature(explosionX, explosionY, 1500.0f);
                            }
                        }
                    }
                }
                return; // Acetylene consumed
            }
        }
    }
    
    // Acetylene is unstable
    if (cell.life == 0) {
        cell.life = 120;
    } else {
        cell.life--;
        if (cell.life < 40 && RandomChance(0.02f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessSulfurDioxide(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // SO2 is heavy and toxic
    if (TryTurbulentMovement(x, y, 0.1f, 0.7f, 3)) return;
    
    // SO2 creates acid rain when it contacts water
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && RandomChance(0.03f)) {
                if (neighborMat->GetName() == "Water") {
                    // SO2 + H2O = H2SO4 (sulfuric acid)
                    MaterialID acidID = materials->GetMaterialID("Acid");
                    if (acidID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, acidID);
                    }
                }
            }
        }
    }
    
    // SO2 persists but slowly converts to acid
    if (cell.life == 0) {
        cell.life = 180;
    } else {
        cell.life--;
        if (cell.life < 40 && RandomChance(0.008f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessCarbonMonoxide(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // CO is deadly but near air density
    if (TryTurbulentMovement(x, y, 0.4f, 0.8f, 4)) return;
    
    // CO is deadly to organics and converts blood
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && RandomChance(0.02f)) {
                if (neighborMat->GetName() == "Blood") {
                    // CO poisoning - blood becomes toxic
                    MaterialID toxicID = materials->GetMaterialID("ToxicGas");
                    if (toxicID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, toxicID);
                    }
                }
                // CO burns to CO2
                else if (neighborMat->GetName() == "Fire") {
                    MaterialID co2ID = materials->GetMaterialID("CarbonDioxide");
                    if (co2ID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, co2ID);
                    }
                    return;
                }
            }
        }
    }
    
    // CO persists dangerously
    if (cell.life == 0) {
        cell.life = 255;
    } else if (RandomChance(0.0003f)) {
        cell.life--;
        if (cell.life < 10) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessNitrousOxide(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // N2O is heavy but supports combustion
    if (TryTurbulentMovement(x, y, 0.2f, 0.7f, 3)) return;
    
    // N2O is an oxidizer - makes fire burn hotter
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire") {
                // N2O supercharges fire
                float currentTemp = m_world->GetTemperature(nx, ny);
                m_world->SetNextTemperature(nx, ny, currentTemp + 20.0f);
                
                // Sometimes creates more fire
                if (RandomChance(0.1f)) {
                    MaterialID fireID = materials->GetMaterialID("Fire");
                    if (fireID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, fireID);
                        m_world->SetNextTemperature(x, y, 800.0f);
                    }
                    return;
                }
            }
        }
    }
    
    // N2O persists
    if (cell.life == 0) {
        cell.life = 200;
    } else {
        cell.life--;
        if (cell.life < 30 && RandomChance(0.01f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessOzone(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Ozone is reactive and heavy
    if (TryTurbulentMovement(x, y, 0.2f, 0.8f, 4)) return;
    
    // Ozone breaks down organics
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && RandomChance(0.05f)) {
                // Ozone oxidizes organics
                if (neighborMat->GetName() == "Wood" || neighborMat->GetName() == "Blood") {
                    MaterialID ashID = materials->GetMaterialID("Ash");
                    if (ashID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, ashID);
                    }
                }
                // Ozone breaks down to oxygen
                else if (neighborMat->GetName() == "Oil") {
                    MaterialID oxygenID = materials->GetMaterialID("Oxygen");
                    if (oxygenID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(x, y, oxygenID);
                    }
                    return;
                }
            }
        }
    }
    
    // Ozone is unstable
    if (cell.life == 0) {
        cell.life = 100;
    } else {
        cell.life--;
        if (cell.life < 20 && RandomChance(0.03f)) {
            // Ozone naturally breaks down to oxygen
            MaterialSystem* materials = m_world->GetMaterialSystem();
            MaterialID oxygenID = materials->GetMaterialID("Oxygen");
            if (oxygenID != MATERIAL_EMPTY) {
                m_world->SetNextMaterial(x, y, oxygenID);
            }
        }
    }
}

void CellularAutomata::ProcessFluorine(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Fluorine is extremely reactive
    if (TryTurbulentMovement(x, y, 0.3f, 0.9f, 4)) return;
    
    // Fluorine attacks EVERYTHING
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && RandomChance(0.08f)) {
                std::string neighborName = neighborMat->GetName();
                
                // Fluorine attacks almost everything
                if (neighborName == "Water") {
                    MaterialID acidID = materials->GetMaterialID("Acid");
                    if (acidID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, acidID);
                    }
                } else if (neighborName == "Wood" || neighborName == "Oil" || neighborName == "Blood") {
                    MaterialID toxicID = materials->GetMaterialID("ToxicGas");
                    if (toxicID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, toxicID);
                    }
                } else if (neighborName == "Metal" || neighborName == "Basalt") {
                    // Fluorine even attacks metals
                    MaterialID ashID = materials->GetMaterialID("Ash");
                    if (ashID != MATERIAL_EMPTY) {
                        m_world->SetNextMaterial(nx, ny, ashID);
                    }
                }
            }
        }
    }
    
    // Fluorine is consumed quickly due to reactivity
    if (cell.life == 0) {
        cell.life = 80;
    } else {
        cell.life--;
        if (cell.life < 30 && RandomChance(0.04f)) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

void CellularAutomata::ProcessXenon(int x, int y) {
    Cell& cell = m_world->GetNextCell(x, y);
    
    // Xenon is very heavy and inert
    if (TryTurbulentMovement(x, y, 0.05f, 0.4f, 2)) return;
    
    // Xenon glows under energy like neon but more intense
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx, ny = y + dy;
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            MaterialSystem* materials = m_world->GetMaterialSystem();
            const Material* neighborMat = materials->GetMaterialPtr(neighborCell.material);
            
            if (neighborMat && neighborMat->GetName() == "Fire") {
                // Xenon produces bright white light (high temperature)
                float currentTemp = m_world->GetTemperature(x, y);
                m_world->SetNextTemperature(x, y, currentTemp + 15.0f);
            }
        }
    }
    
    // Xenon is very stable due to its density
    if (cell.life == 0) {
        cell.life = 255;
    } else if (RandomChance(0.00005f)) {
        cell.life--;
        if (cell.life < 2) {
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
        }
    }
}

bool CellularAutomata::TryPowderMove(int fromX, int fromY, int toX, int toY, float density) {
    // Check bounds
    if (!m_world->IsValidPosition(toX, toY)) {
        return false;
    }
    
    // Check if destination is empty
    if (IsEmpty(toX, toY)) {
        return TryMove(fromX, fromY, toX, toY);
    }
    
    // Check density-based displacement
    const Cell& targetCell = m_world->GetCell(toX, toY);
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* targetMaterial = materials->GetMaterialPtr(targetCell.material);
    
    if (targetMaterial) {
        float targetDensity = targetMaterial->GetPhysicalProps().density;
        MaterialBehavior targetBehavior = targetMaterial->GetBehavior();
        
        // FIXED: More realistic powder displacement based on density
        
        // Powder-powder interactions: powders should stack on top of each other
        if (targetBehavior == MaterialBehavior::Powder) {
            // NO DISPLACEMENT: Powders never displace other powders in real life
            // They simply stack on top of each other regardless of density
            return false;
        }
        
        // Powder-liquid interactions: sink slowly through liquids based on density
        if (targetBehavior == MaterialBehavior::Liquid) {
            if (density > targetDensity + 0.05f) {
                // Dense powder sinks through lighter liquid, but slowly
                // Add random chance to make sinking gradual instead of instant
                if (RandomChance(0.3f)) { // 30% chance per frame for slower sinking
                    return TryMove(fromX, fromY, toX, toY);
                }
                return false; // Don't move this frame
            } else {
                // Light powder floats on dense liquid (ash on molten metal)
                return false;
            }
        }
        
        // Static materials cannot be displaced
        if (targetBehavior == MaterialBehavior::Static) {
            return false;
        }
    }
    
    return false;
}

bool CellularAutomata::ShouldPowderSlide(int x, int y, float angleOfRepose) {
    // Count height differences to determine if sliding should occur
    int leftHeight = 0, rightHeight = 0;
    
    // Check pile height to the left
    for (int i = 1; i <= 3; ++i) {
        if (m_world->IsValidPosition(x - i, y) && !IsEmpty(x - i, y)) {
            leftHeight++;
        } else {
            break;
        }
    }
    
    // Check pile height to the right
    for (int i = 1; i <= 3; ++i) {
        if (m_world->IsValidPosition(x + i, y) && !IsEmpty(x + i, y)) {
            rightHeight++;
        } else {
            break;
        }
    }
    
    // Check height above current position
    int currentHeight = 0;
    for (int i = 1; i <= 5; ++i) {
        if (m_world->IsValidPosition(x, y - i) && !IsEmpty(x, y - i)) {
            currentHeight++;
        } else {
            break;
        }
    }
    
    // Calculate if angle exceeds repose angle
    float heightDiff = static_cast<float>(std::abs(leftHeight - rightHeight));
    return (heightDiff > angleOfRepose * currentHeight) && RandomChance(0.3f);
}

int CellularAutomata::GetSlideDirection(int x, int y) {
    // Determine which direction has more space to slide into
    bool canSlideLeft = m_world->IsValidPosition(x - 1, y) && IsEmpty(x - 1, y);
    bool canSlideRight = m_world->IsValidPosition(x + 1, y) && IsEmpty(x + 1, y);
    
    if (canSlideLeft && canSlideRight) {
        // Choose random direction if both are available
        return RandomDirection();
    } else if (canSlideLeft) {
        return -1;
    } else if (canSlideRight) {
        return 1;
    }
    
    return 0; // No sliding possible
}

bool CellularAutomata::IsPowderStable(int x, int y) {
    // Check if powder has solid support below (not just floating)
    if (!m_world->IsValidPosition(x, y + 1)) {
        return false; // At bottom edge, consider unstable
    }
    
    const Cell& belowCell = m_world->GetCell(x, y + 1);
    if (belowCell.material == MATERIAL_EMPTY) {
        return false; // Nothing below, definitely not stable
    }
    
    // Check if there's solid material below (powder, liquid, or static)
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* belowMaterial = materials->GetMaterialPtr(belowCell.material);
    if (!belowMaterial) {
        return false;
    }
    
    MaterialBehavior belowBehavior = belowMaterial->GetBehavior();
    
    // Stable if supported by static material or dense enough material
    if (belowBehavior == MaterialBehavior::Static) {
        return true; // Always stable on static materials
    }
    
    if (belowBehavior == MaterialBehavior::Powder) {
        // Check if the pile below is reasonably tall (suggests stability)
        int supportHeight = 0;
        for (int checkY = y + 1; checkY < static_cast<int>(m_world->GetHeight()) && checkY < y + 4; ++checkY) {
            if (m_world->IsValidPosition(x, checkY) && !IsEmpty(x, checkY)) {
                supportHeight++;
            } else {
                break;
            }
        }
        return supportHeight >= 2; // Need at least 2 cells of support below
    }
    
    if (belowBehavior == MaterialBehavior::Liquid) {
        // Only stable if powder is much denser than liquid (won't sink easily)
        const Cell& currentCell = m_world->GetCell(x, y);
        const Material* currentMaterial = materials->GetMaterialPtr(currentCell.material);
        if (currentMaterial) {
            float currentDensity = currentMaterial->GetPhysicalProps().density;
            float belowDensity = belowMaterial->GetPhysicalProps().density;
            return currentDensity <= belowDensity + 0.1f; // Stable if densities are close
        }
    }
    
    return false; // Default to unstable
}

float CellularAutomata::GetPowderAngleOfRepose(const std::string& materialName) {
    // Return angle of repose values for different powder types
    if (materialName == "Sand") return 0.6f;
    if (materialName == "Gravel") return 0.8f;
    if (materialName == "Dirt") return 0.4f;
    if (materialName == "Clay") return 0.3f;
    if (materialName == "Ash") return 0.5f;
    if (materialName == "Dust") return 0.2f;
    if (materialName == "BoneDust") return 0.3f;
    if (materialName == "Sludge") return 0.1f;
    if (materialName == "Gunpowder") return 0.4f;
    if (materialName == "Snow") return 0.7f;
    
    // Default value for unknown powders
    return 0.5f;
}

float CellularAutomata::GetPowderCohesion(const std::string& materialName) {
    // Return cohesion values (0.0 = no cohesion, 1.0 = maximum cohesion)
    if (materialName == "Sand") return 0.1f;
    if (materialName == "Gravel") return 0.0f;
    if (materialName == "Dirt") return 0.3f;
    if (materialName == "Clay") return 0.8f;
    if (materialName == "Ash") return 0.2f;
    if (materialName == "Dust") return 0.1f;
    if (materialName == "BoneDust") return 0.4f;
    if (materialName == "Sludge") return 0.9f;
    if (materialName == "Gunpowder") return 0.2f;
    if (materialName == "Snow") return 0.6f;
    
    // Default value for unknown powders
    return 0.2f;
}

void CellularAutomata::CreateExplosion(int centerX, int centerY, float power, float radius) {
    if (!m_world) return;
    
    // Create explosion pattern in a circle around the center
    int radiusInt = static_cast<int>(radius + 0.5f);
    
    for (int dy = -radiusInt; dy <= radiusInt; ++dy) {
        for (int dx = -radiusInt; dx <= radiusInt; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            
            if (!m_world->IsValidPosition(x, y)) continue;
            
            // Calculate distance from explosion center
            float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (distance > radius) continue;
            
            // Calculate explosion force (stronger at center)
            float force = power * (1.0f - distance / radius);
            
            MaterialID currentMaterial = m_world->GetMaterial(x, y);
            if (currentMaterial == MATERIAL_EMPTY) continue;
            
            // Check if material can be destroyed by this explosion
            if (CanDestroyMaterial(currentMaterial, force)) {
                // Destroy material - replace with appropriate debris or empty space
                MaterialSystem* materialSystem = m_world->GetMaterialSystem();
                if (materialSystem) {
                    const Material* mat = materialSystem->GetMaterialPtr(currentMaterial);
                    if (mat && mat->GetPhysicalProps().hardness < force) {
                        // Spectacular explosion effects - more fire, burning layers, blackening
                        if (force > 3.0f) {
                            // Intense explosions: Create fire and add burning effects to surrounding area
                            m_world->SetNextMaterial(x, y, materialSystem->GetMaterialID("Fire"));
                            
                            // Add burning effect to nearby materials
                            for (int bdy = -2; bdy <= 2; ++bdy) {
                                for (int bdx = -2; bdx <= 2; ++bdx) {
                                    int fx = x + bdx;
                                    int fy = y + bdy;
                                    if (m_world->IsValidPosition(fx, fy)) {
                                        MaterialID nearMaterial = m_world->GetMaterial(fx, fy);
                                        if (nearMaterial != MATERIAL_EMPTY && nearMaterial != materialSystem->GetMaterialID("Fire")) {
                                            // Add burning effect layer
                                            uint8_t burnIntensity = static_cast<uint8_t>(200 - (bdx*bdx + bdy*bdy) * 20);
                                            if (burnIntensity > 50) {
                                                m_world->SetEffect(fx, fy, EffectLayer::Burning, burnIntensity, 180);
                                            }
                                        }
                                    }
                                }
                            }
                        } else if (force > 1.5f) {
                            // Medium explosions: Fire + blackening effects
                            if (RandomChance(0.7f)) {
                                m_world->SetNextMaterial(x, y, materialSystem->GetMaterialID("Fire"));
                            } else {
                                m_world->SetNextMaterial(x, y, materialSystem->GetMaterialID("Smoke"));
                            }
                            
                            // Add blackening effect to show blast damage
                            m_world->SetEffect(x, y, EffectLayer::Blackened, 150, 240);
                        } else {
                            // Weak explosions: Debris + blackening
                            if (mat->GetBehavior() == MaterialBehavior::Static) {
                                MaterialID ashID = materialSystem->GetMaterialID("Ash");
                                if (ashID != MATERIAL_EMPTY) {
                                    m_world->SetNextMaterial(x, y, ashID);
                                } else {
                                    m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
                                }
                            } else {
                                m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
                            }
                            
                            // Light blackening effect
                            m_world->SetEffect(x, y, EffectLayer::Blackened, 80, 120);
                        }
                    }
                }
            } else {
                // Material survived explosion - might create fire nearby if it's flammable
                if (force > 2.0f && RandomChance(0.3f)) {
                    MaterialSystem* materialSystem = m_world->GetMaterialSystem();
                    if (materialSystem) {
                        MaterialID fireID = materialSystem->GetMaterialID("Fire");
                        if (fireID != MATERIAL_EMPTY) {
                            // Try to place fire in adjacent empty spaces
                            for (const auto& offset : NEIGHBOR_OFFSETS) {
                                int fx = x + offset.first;
                                int fy = y + offset.second;
                                if (m_world->IsValidPosition(fx, fy) && 
                                    m_world->GetMaterial(fx, fy) == MATERIAL_EMPTY) {
                                    m_world->SetNextMaterial(fx, fy, fireID);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

bool CellularAutomata::CanDestroyMaterial(MaterialID material, float explosivePower) const {
    if (!m_world || material == MATERIAL_EMPTY) return false;
    
    MaterialSystem* materialSystem = m_world->GetMaterialSystem();
    if (!materialSystem) return false;
    
    const Material* mat = materialSystem->GetMaterialPtr(material);
    if (!mat) return false;
    
    const auto& physProps = mat->GetPhysicalProps();
    
    // Material can be destroyed if explosion power exceeds its resistance
    float totalResistance = physProps.hardness + physProps.explosiveResistance;
    return explosivePower > totalResistance;
}

} // namespace BGE