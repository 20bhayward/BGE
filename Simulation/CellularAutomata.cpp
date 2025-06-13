#include "CellularAutomata.h"
#include "SimulationWorld.h"
#include "Materials/MaterialSystem.h"
#include <iostream>

namespace BGE {

CellularAutomata::CellularAutomata(SimulationWorld* world) 
    : m_world(world) {
}

CellularAutomata::~CellularAutomata() = default;

bool CellularAutomata::IsEmpty(int x, int y) const {
    return m_world->GetMaterial(x, y) == MATERIAL_EMPTY;
}

void CellularAutomata::ProcessReactions(int x, int y, float deltaTime) {
    (void)x; (void)y; (void)deltaTime;
    // TODO: Implement chemical reactions between materials
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
    // Powder materials (sand, dirt) fall due to gravity
    // and pile up at angles of repose
    
    // Try to fall straight down first
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    // If can't fall straight, try diagonal movement
    // Add some randomness for more natural behavior
    int direction = (rand() % 2) * 2 - 1; // -1 or 1
    
    // Try to slide down diagonally (natural avalanche behavior)
    if (TryMove(x, y, x + direction, y + 1)) {
        return;
    }
    if (TryMove(x, y, x - direction, y + 1)) {
        return;
    }
    
    // Check for pile stability - powders have angle of repose
    // If too steep, slide sideways
    int pileHeight = GetPileHeight(x, y);
    if (pileHeight > 2) { // Simple angle of repose simulation
        // Try to spread horizontally to stabilize pile
        if (IsEmpty(x + direction, y)) {
            TryMove(x, y, x + direction, y);
        } else if (IsEmpty(x - direction, y)) {
            TryMove(x, y, x - direction, y);
        }
    }
}

void CellularAutomata::ProcessLiquid(int x, int y) {
    // Liquids fall and flow to find their level - MUCH MORE FLUID BEHAVIOR
    
    // 1. GRAVITY: Try to fall straight down first (high priority)
    if (TryMove(x, y, x, y + 1)) {
        return;
    }
    
    // 2. DIAGONAL FALLING: Try diagonal falls (liquids flow around obstacles)
    int direction = (rand() % 2) * 2 - 1; // -1 or 1
    if (TryMove(x, y, x + direction, y + 1)) {
        return;
    }
    if (TryMove(x, y, x - direction, y + 1)) {
        return;
    }
    
    // 3. HORIZONTAL FLOW: Liquids spread much more aggressively
    // Calculate liquid pressure (how much liquid is stacked above)
    int liquidHeight = GetLiquidColumn(x, y);
    int leftHeight = GetLiquidColumn(x - 1, y);
    int rightHeight = GetLiquidColumn(x + 1, y);
    
    // Flow towards areas with less liquid (pressure equalization)
    bool shouldFlowLeft = leftHeight < liquidHeight - 1;
    bool shouldFlowRight = rightHeight < liquidHeight - 1;
    
    // Higher probability to flow horizontally for more fluid behavior
    int flowChance = 60; // 60% base chance
    if (liquidHeight > 3) flowChance = 80; // More pressure = more flow
    if (liquidHeight > 6) flowChance = 95; // High pressure = almost always flow
    
    if (rand() % 100 < flowChance) {
        // Prefer flowing towards lower liquid levels
        if (shouldFlowLeft && shouldFlowRight) {
            // Flow towards the side with less liquid
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
        } else {
            // No pressure difference, random flow for fluidity
            if (TryMove(x, y, x + direction, y)) return;
            if (TryMove(x, y, x - direction, y)) return;
        }
    }
    
    // 4. ADDITIONAL FLOW ATTEMPTS: Try more positions for fluidity
    if (rand() % 100 < 30) { // 30% chance for extra flow attempts
        // Try flowing in multiple directions
        for (int i = 0; i < 2; ++i) {
            int testDir = (rand() % 2) * 2 - 1;
            if (TryMove(x, y, x + testDir, y)) {
                return;
            }
        }
    }
    
    // If no movement is possible, liquid stays in place (CONSERVATION!)
}

void CellularAutomata::ProcessGas(int x, int y) {
    // Get gas properties for density-based behavior
    const Cell& cell = m_world->GetCell(x, y);
    MaterialSystem* materials = m_world->GetMaterialSystem();
    const Material* material = materials->GetMaterialPtr(cell.material);
    
    if (!material) return;
    
    std::string name = material->GetName();
    
    // Different gas behaviors based on type and density
    int riseChance, spreadChance, expansionRange;
    
    if (name == "NaturalGas") {
        // Natural gas: very light, rises quickly, spreads fast
        riseChance = 90;  // 90% chance to try rising
        spreadChance = 70; // 70% chance to spread horizontally
        expansionRange = 3;
    } else if (name == "ThickGas") {
        // Thick gas: heavier, rises slower, spreads more horizontally
        riseChance = 40;  // 40% chance to try rising
        spreadChance = 80; // 80% chance to spread horizontally
        expansionRange = 2;
    } else if (name == "Smoke") {
        // Smoke: rises moderately, disperses in all directions
        riseChance = 60;  // 60% chance to try rising
        spreadChance = 85; // 85% chance to spread
        expansionRange = 4; // Wide dispersal
    } else if (name == "PoisonGas") {
        // Poison gas: medium rise, moderate spread, persistent
        riseChance = 50;  // 50% chance to try rising
        spreadChance = 60; // 60% chance to spread
        expansionRange = 2;
    } else {
        // Default gas behavior (steam, etc.)
        riseChance = 70;
        spreadChance = 75;
        expansionRange = 2;
    }
    
    // Try to rise up (gases are buoyant) - probability based on gas type
    if (rand() % 100 < riseChance) {
        if (TryMove(x, y, x, y - 1)) {
            return;
        }
        
        // Try to rise diagonally if straight up is blocked
        int direction = (rand() % 2) * 2 - 1; // -1 or 1
        if (TryMove(x, y, x + direction, y - 1)) {
            return;
        }
        if (TryMove(x, y, x - direction, y - 1)) {
            return;
        }
    }
    
    // Horizontal spreading - probability based on gas type
    if (rand() % 100 < spreadChance) {
        int direction = (rand() % 2) * 2 - 1;
        if (TryMove(x, y, x + direction, y)) {
            return;
        }
        if (TryMove(x, y, x - direction, y)) {
            return;
        }
    }
    
    // Gas expansion - range and probability based on gas type
    if (rand() % 100 < 30) { // 30% chance for expansion
        for (int dx = -expansionRange; dx <= expansionRange; ++dx) {
            for (int dy = -expansionRange; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                if (rand() % 20 == 0) { // Random expansion within range
                    if (TryMove(x, y, x + dx, y + dy)) {
                        return;
                    }
                }
            }
        }
    }
    
    // Gas stays in place if no movement possible (conservation!)
}

void CellularAutomata::ProcessFire(int x, int y) {
    // Fire is a gas that rises, spreads aggressively, and consumes fuel
    
    // Fire rises like a gas due to buoyancy (faster than regular gas)
    if (TryMove(x, y, x, y - 1)) {
        return;
    }
    
    // Try to rise diagonally
    int direction = (rand() % 2) * 2 - 1;
    if (TryMove(x, y, x + direction, y - 1)) {
        return;
    }
    if (TryMove(x, y, x - direction, y - 1)) {
        return;
    }
    
    // Fire spreads to combustible materials and empty spaces
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (!m_world->IsValidPosition(nx, ny)) continue;
            
            const Cell& neighborCell = m_world->GetCell(nx, ny);
            
            // Fire doesn't spread to completely empty space - it needs fuel or existing heat
            // Remove this block to prevent fire spawning in void
            
            // Ignite combustible materials
            if (neighborCell.material != MATERIAL_EMPTY) {
                // Increase neighbor temperature aggressively
                float currentTemp = m_world->GetTemperature(nx, ny);
                m_world->SetNextTemperature(nx, ny, currentTemp + 100.0f);
                
                // Check if material ignites
                MaterialSystem* materials = m_world->GetMaterialSystem();
                const Material* mat = materials->GetMaterialPtr(neighborCell.material);
                if (mat) {
                    float ignitionPoint = mat->GetThermalProps().ignitionPoint;
                    
                    // Wood burns, but not too easily to prevent runaway fires
                    if (neighborCell.material == GetWoodMaterialID() && currentTemp > 300.0f && rand() % 100 == 0) {
                        m_world->SetNextMaterial(nx, ny, GetFireMaterialID());
                        m_world->SetNextTemperature(nx, ny, 800.0f);
                    }
                    // Other materials need even higher temperature
                    else if (currentTemp > ignitionPoint && rand() % 200 == 0) {
                        m_world->SetNextMaterial(nx, ny, GetFireMaterialID());
                        m_world->SetNextTemperature(nx, ny, 800.0f);
                    }
                }
            }
        }
    }
    
    // Fire burns out over time, but lives much longer
    if (rand() % 1000 == 0) { // Much slower burnout - fire should be more persistent
        // Most fire just burns out to empty space, not steam
        if (rand() % 10 == 0) { // Only 10% chance to become steam
            MaterialSystem* materials = m_world->GetMaterialSystem();
            MaterialID steamID = materials->GetMaterialID("Steam");
            if (steamID != MATERIAL_EMPTY) {
                m_world->SetNextMaterial(x, y, steamID);
                m_world->SetNextTemperature(x, y, 150.0f);
            } else {
                m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
            }
        } else {
            // Fire just burns out to empty space
            m_world->SetNextMaterial(x, y, MATERIAL_EMPTY);
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
        
        // Copy other properties
        m_world->SetNextTemperature(toX, toY, fromCell.temperature);
        m_world->SetNextTemperature(fromX, fromY, 20.0f);
        
        return true;
    }
    
    // Handle density-based displacement for non-empty destinations
    if (toCell.material != MATERIAL_EMPTY && nextToCell.material == MATERIAL_EMPTY) {
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
            
            // ENHANCED INTERACTION RULES (FIXED):
            
            // RULE 1: Dense materials fall THROUGH less dense materials (gravity)
            if (fromDensity > toDensity + 0.1f) {
                // Heavy powders (sand) fall through gases, liquids, fire
                if (fromBehavior == MaterialBehavior::Powder) {
                    if (toBehavior == MaterialBehavior::Gas || toBehavior == MaterialBehavior::Liquid || toBehavior == MaterialBehavior::Fire) {
                        SwapCells(fromX, fromY, toX, toY);
                        return true;
                    }
                }
                
                // Heavy liquids fall through lighter liquids and gases
                if (fromBehavior == MaterialBehavior::Liquid) {
                    if (toBehavior == MaterialBehavior::Gas || toBehavior == MaterialBehavior::Liquid) {
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
            }
            
            // RULE 3: Special liquid displacement rules  
            if (fromBehavior == MaterialBehavior::Liquid && toBehavior == MaterialBehavior::Powder) {
                // Liquids can displace powders regardless of density (water flows through sand)
                SwapCells(fromX, fromY, toX, toY);
                return true;
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
    
    // Write swapped data to next grid
    m_world->SetNextMaterial(x1, y1, cell2.material);
    m_world->SetNextMaterial(x2, y2, cell1.material);
    m_world->SetNextTemperature(x1, y1, cell2.temperature);
    m_world->SetNextTemperature(x2, y2, cell1.temperature);
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

} // namespace BGE