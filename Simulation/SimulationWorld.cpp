#include "SimulationWorld.h"
#include "Materials/MaterialSystem.h"
#include "World/ChunkManager.h"
#include "CellularAutomata.h"
#include "Physics/PhysicsWorld.h"
#include "../Core/Threading/ThreadPool.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <set>

namespace BGE {

SimulationWorld::SimulationWorld(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {
    
    std::cout << "Creating simulation world: " << width << "x" << height << std::endl;
    
    // Allocate grids
    size_t cellCount = static_cast<size_t>(width) * height;
    m_currentGrid.resize(cellCount);
    m_nextGrid.resize(cellCount);
    
    // Allocate pixel buffer (RGBA)
    m_pixelBuffer.resize(cellCount * 4);
    m_dirtyRegions.resize((width / 32 + 1) * (height / 32 + 1), true);
    
    // Initialize systems
    m_materialSystem = std::make_unique<MaterialSystem>();
    m_chunkManager = std::make_unique<ChunkManager>(this);
    m_cellularAutomata = std::make_unique<CellularAutomata>(this);
    
    if (m_maxThreads == 0) {
        m_maxThreads = std::thread::hardware_concurrency();
    }
    
    if (m_multithreading && m_maxThreads > 1) {
        m_threadPool = std::make_unique<ThreadPool>(m_maxThreads);
        std::cout << "Using " << m_maxThreads << " threads for simulation" << std::endl;
    }
    
    // Clear world
    Clear();
    
    std::cout << "Simulation world created successfully!" << std::endl;
}

SimulationWorld::~SimulationWorld() {
    std::cout << "Destroying simulation world..." << std::endl;
}

void SimulationWorld::Update(float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Check if simulation should run
    bool shouldUpdate = !m_paused || m_stepOnce;
    if (m_stepOnce) {
        m_stepOnce = false; // Reset step flag
    }
    
    if (shouldUpdate) {
        deltaTime *= m_simulationSpeed;
        
        // Count water before update
        uint32_t waterBefore = 0;
        for (const auto& cell : m_currentGrid) {
            if (cell.material == 2) ++waterBefore;
        }
        
        // Update cellular automata
        UpdateCellularAutomata(deltaTime);
        
        // Count water in next grid after CA update
        uint32_t waterAfterCA = 0;
        for (const auto& cell : m_nextGrid) {
            if (cell.material == 2) ++waterAfterCA;
        }
        
        // Update temperature
        UpdateTemperature(deltaTime);
        
        // Update reactions
        UpdateReactions(deltaTime);
        
        // Update effects
        UpdateEffects(deltaTime);
        
        // Count water in next grid after all updates
        uint32_t waterAfterAll = 0;
        for (const auto& cell : m_nextGrid) {
            if (cell.material == 2) ++waterAfterAll;
        }
        
        // Swap buffers if needed
        if (m_swapBuffers.exchange(false)) {
            m_currentGrid.swap(m_nextGrid);
            
            // Debug water count changes
            static int logCount = 0;
            if ((waterBefore != waterAfterCA || waterAfterCA != waterAfterAll) && logCount++ < 10) {
                std::cout << "Water count: Before=" << waterBefore 
                         << " AfterCA=" << waterAfterCA 
                         << " AfterAll=" << waterAfterAll << std::endl;
            }
        }
        
        // Update statistics
        ++m_updateCount;
        
        if (m_updateCount % 60 == 0 && !m_paused) {
            std::cout << "Simulation update " << m_updateCount 
                      << ", time: " << (m_lastUpdateTime * 1000.0f) << "ms" << std::endl;
        }
    }
    
    // Always update pixel buffer for rendering (even when paused)
    UpdatePixelBuffer();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_lastUpdateTime = std::chrono::duration<float>(endTime - startTime).count();
}

void SimulationWorld::Reset() {
    std::cout << "Resetting simulation world..." << std::endl;
    Clear();
    m_updateCount = 0;
}

void SimulationWorld::Clear() {
    // Clear all cells
    for (auto& cell : m_currentGrid) {
        cell = Cell{};
    }
    for (auto& cell : m_nextGrid) {
        cell = Cell{};
    }
    
    // Clear pixel buffer
    std::fill(m_pixelBuffer.begin(), m_pixelBuffer.end(), 0);
    
    // Mark all regions dirty
    std::fill(m_dirtyRegions.begin(), m_dirtyRegions.end(), true);
    
    m_activeCells = 0;
}

bool SimulationWorld::IsValidPosition(int x, int y) const {
    return x >= 0 && y >= 0 && x < static_cast<int>(m_width) && y < static_cast<int>(m_height);
}

const Cell& SimulationWorld::GetCell(int x, int y) const {
    if (!IsValidPosition(x, y)) {
        static const Cell emptyCell{};
        return emptyCell;
    }
    return m_currentGrid[CoordToIndex(x, y, m_width)];
}

void SimulationWorld::SetMaterial(int x, int y, MaterialID material) {
    if (!IsValidPosition(x, y)) return;
    
    int index = CoordToIndex(x, y, m_width);
    MaterialID oldMaterial = m_currentGrid[index].material;
    m_currentGrid[index].material = material;
    
    // CRITICAL DEBUG: Track every water change in current grid too
    if (oldMaterial == 2 && material != 2) {
        static int deleteCount = 0;
        if (deleteCount++ < 5) {
            std::cout << "CURRENT GRID WATER DELETED at (" << x << "," << y << ") changed from " << oldMaterial << " to " << material << std::endl;
        }
    }
    
    // Mark chunk as active
    if (m_chunkManager) {
        int chunkX, chunkY;
        chunkX = x / CHUNK_SIZE;
        chunkY = y / CHUNK_SIZE;
        m_chunkManager->MarkChunkDirty(chunkX, chunkY);
    }
    
    // Update active cell count
    if (material != MATERIAL_EMPTY) {
        ++m_activeCells;
    }
}

void SimulationWorld::SetTemperature(int x, int y, float temperature) {
    if (!IsValidPosition(x, y)) return;
    
    int index = CoordToIndex(x, y, m_width);
    m_currentGrid[index].temperature = temperature;
}

void SimulationWorld::SetEffect(int x, int y, EffectLayer effect, uint8_t intensity, uint8_t duration) {
    if (!IsValidPosition(x, y)) return;
    
    int index = CoordToIndex(x, y, m_width);
    m_currentGrid[index].effectLayer = effect;
    m_currentGrid[index].effectIntensity = intensity;
    m_currentGrid[index].effectTimer = duration;
}

void SimulationWorld::ClearEffect(int x, int y) {
    if (!IsValidPosition(x, y)) return;
    
    int index = CoordToIndex(x, y, m_width);
    m_currentGrid[index].effectLayer = EffectLayer::None;
    m_currentGrid[index].effectIntensity = 0;
    m_currentGrid[index].effectTimer = 0;
}

EffectLayer SimulationWorld::GetEffect(int x, int y) const {
    return GetCell(x, y).effectLayer;
}

uint8_t SimulationWorld::GetEffectIntensity(int x, int y) const {
    return GetCell(x, y).effectIntensity;
}

MaterialID SimulationWorld::GetMaterial(int x, int y) const {
    return GetCell(x, y).material;
}

float SimulationWorld::GetTemperature(int x, int y) const {
    return GetCell(x, y).temperature;
}

void SimulationWorld::SetNextMaterial(int x, int y, MaterialID material) {
    if (!IsValidPosition(x, y)) return;
    
    int index = CoordToIndex(x, y, m_width);
    MaterialID oldMaterial = m_nextGrid[index].material;
    m_nextGrid[index].material = material;
    
    // CRITICAL DEBUG: Track every water change
    if (oldMaterial == 2 && material != 2) {
        static int deleteCount = 0;
        if (deleteCount++ < 10) {
            std::cout << "WATER DELETED at (" << x << "," << y << ") changed from " << oldMaterial << " to " << material << std::endl;
        }
    }
    if (oldMaterial != 2 && material == 2) {
        static int createCount = 0;
        if (createCount++ < 10) {
            std::cout << "WATER CREATED at (" << x << "," << y << ") changed from " << oldMaterial << " to " << material << std::endl;
        }
    }
}

void SimulationWorld::SetNextTemperature(int x, int y, float temperature) {
    if (!IsValidPosition(x, y)) return;
    
    int index = CoordToIndex(x, y, m_width);
    m_nextGrid[index].temperature = temperature;
}

Cell& SimulationWorld::GetNextCell(int x, int y) {
    static Cell emptyCell{};
    if (!IsValidPosition(x, y)) {
        return emptyCell;
    }
    return m_nextGrid[CoordToIndex(x, y, m_width)];
}

void SimulationWorld::FillRegion(int x1, int y1, int x2, int y2, MaterialID material) {
    for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
            SetMaterial(x, y, material);
        }
    }
}

void SimulationWorld::FillCircle(int centerX, int centerY, int radius, MaterialID material) {
    for (int y = centerY - radius; y <= centerY + radius; ++y) {
        for (int x = centerX - radius; x <= centerX + radius; ++x) {
            int dx = x - centerX;
            int dy = y - centerY;
            if (dx * dx + dy * dy <= radius * radius) {
                SetMaterial(x, y, material);
            }
        }
    }
}

bool SimulationWorld::IsRegionDirty(int x, int y, int width, int height) const {
    (void)width; (void)height;
    // Simplified dirty checking
    int regionX = x / 32;
    int regionY = y / 32;
    int regionIndex = regionY * (m_width / 32 + 1) + regionX;
    
    if (regionIndex >= 0 && regionIndex < static_cast<int>(m_dirtyRegions.size())) {
        return m_dirtyRegions[regionIndex];
    }
    return true;
}

void SimulationWorld::MarkRegionClean(int x, int y, int width, int height) {
    (void)width; (void)height;
    // Mark region as clean
    int regionX = x / 32;
    int regionY = y / 32;
    int regionIndex = regionY * (m_width / 32 + 1) + regionX;
    
    if (regionIndex >= 0 && regionIndex < static_cast<int>(m_dirtyRegions.size())) {
        m_dirtyRegions[regionIndex] = false;
    }
}

void SimulationWorld::SetMaxThreads(uint32_t threads) {
    m_maxThreads = threads;
    if (m_threadPool) {
        m_threadPool->Resize(threads);
    }
}

void SimulationWorld::UpdateCellularAutomata(float deltaTime) {
    if (m_cellularAutomata) {
        // CRITICAL FIX: Initialize next grid properly to prevent mass loss
        // Copy the current grid to preserve all materials
        m_nextGrid = m_currentGrid;
        
        // Process all active cells using cellular automata update
        m_cellularAutomata->Update(deltaTime);
        
        // Count active cells
        uint32_t activeCells = 0;
        for (const auto& cell : m_nextGrid) {
            if (cell.material != MATERIAL_EMPTY) {
                ++activeCells;
            }
        }
        m_activeCells = activeCells;
        
        // Mark that we need to swap buffers
        m_swapBuffers = true;
    }
}

void SimulationWorld::UpdateTemperature(float deltaTime) {
    // CRITICAL FIX: Work with the NEXT grid to avoid overwriting material changes
    // Create a copy of next grid for temperature calculations
    std::vector<Cell> tempGrid = m_nextGrid;
    
    // Simple temperature diffusion - work entirely within the next grid
    for (uint32_t y = 1; y < m_height - 1; ++y) {
        for (uint32_t x = 1; x < m_width - 1; ++x) {
            int index = CoordToIndex(x, y, m_width);
            const Cell& cell = tempGrid[index];
            if (cell.material == MATERIAL_EMPTY) continue;
            
            float avgTemp = 0.0f;
            int neighbors = 0;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = x + dx, ny = y + dy;
                    if (IsValidPosition(nx, ny)) {
                        int neighborIndex = CoordToIndex(nx, ny, m_width);
                        const Cell& neighbor = tempGrid[neighborIndex];
                        if (neighbor.material != MATERIAL_EMPTY) {
                            avgTemp += neighbor.temperature;
                            ++neighbors;
                        }
                    }
                }
            }
            
            if (neighbors > 0) {
                avgTemp /= neighbors;
                float newTemp = cell.temperature + (avgTemp - cell.temperature) * TEMPERATURE_DIFFUSION * deltaTime;
                
                // ONLY update temperature, preserve material that CA already set
                m_nextGrid[index].temperature = newTemp;
            }
        }
    }
}

void SimulationWorld::UpdateReactions(float deltaTime) {
    // Process material reactions
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            const Cell& cell = GetCell(x, y);
            if (cell.material == MATERIAL_EMPTY) continue;
            
            if (m_materialSystem && m_cellularAutomata) {
                m_cellularAutomata->ProcessReactions(x, y, deltaTime);
            }
        }
    }
}

void SimulationWorld::UpdateEffects(float deltaTime) {
    // Update effect timers and fade temporary effects
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            Cell& cell = m_currentGrid[CoordToIndex(x, y, m_width)];
            
            if (cell.effectLayer != EffectLayer::None && cell.effectTimer > 0) {
                // Decrease timer
                if (cell.effectTimer > static_cast<uint8_t>(deltaTime * 60)) {
                    cell.effectTimer -= static_cast<uint8_t>(deltaTime * 60);
                } else {
                    cell.effectTimer = 0;
                }
                
                // Fade intensity based on remaining time
                if (cell.effectTimer == 0) {
                    // Effect expired - clear it
                    cell.effectLayer = EffectLayer::None;
                    cell.effectIntensity = 0;
                } else {
                    // Fade effect over time
                    float fadeRatio = static_cast<float>(cell.effectTimer) / 255.0f;
                    cell.effectIntensity = static_cast<uint8_t>(cell.effectIntensity * fadeRatio);
                }
            }
        }
    }
}

void SimulationWorld::UpdatePixelBuffer() {
    // Convert world state to pixel buffer
    static bool debugPrinted = false;
    uint32_t nonEmptyCount = 0;
    
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            const Cell& cell = GetCell(x, y);
            uint32_t color;
            
            // Get the actual material color with effect layers
            color = MaterialToColor(cell.material, cell.temperature, x, y);
            
            // Apply effect layer blending
            if (cell.effectLayer != EffectLayer::None && cell.effectIntensity > 0) {
                color = BlendEffectLayer(color, cell.effectLayer, cell.effectIntensity);
            }
            
            if (cell.material != MATERIAL_EMPTY) {
                nonEmptyCount++;
            }
            
            // Flip Y coordinate for OpenGL (Y=0 at bottom in OpenGL, at top in simulation)
            int flippedY = m_height - 1 - y;
            int pixelIndex = CoordToIndex(x, flippedY, m_width) * 4;
            m_pixelBuffer[pixelIndex + 0] = (color >> 0) & 0xFF;  // R
            m_pixelBuffer[pixelIndex + 1] = (color >> 8) & 0xFF;  // G
            m_pixelBuffer[pixelIndex + 2] = (color >> 16) & 0xFF; // B
            m_pixelBuffer[pixelIndex + 3] = (color >> 24) & 0xFF; // A
        }
    }
    
    // Clean console output - remove debug spam
    static int updateCounter = 0;
    if (++updateCounter % 300 == 0 && nonEmptyCount > 0) {
        std::cout << "Simulation active: " << nonEmptyCount << " particles" << std::endl;
    }
}

uint32_t SimulationWorld::MaterialToColor(MaterialID material, float temperature, int x, int y) const {
    if (material == MATERIAL_EMPTY) {
        return 0x00000000; // Transparent
    }
    
    // Get material color from material system
    if (m_materialSystem) {
        const Material* mat = m_materialSystem->GetMaterialPtr(material);
        if (mat) {
            uint32_t baseColor = mat->GetColor();
            const VisualProperties& visualProps = mat->GetVisualProps();
            
            // Debug: Print material colors once
            static std::set<MaterialID> printedMaterials;
            if (printedMaterials.find(material) == printedMaterials.end()) {
                std::cout << "Material " << material << " color: 0x" << std::hex << baseColor << std::dec << std::endl;
                printedMaterials.insert(material);
            }
            
            // Apply visual pattern
            uint32_t finalColor = ApplyVisualPattern(baseColor, visualProps, x, y);
            
            // Modify color based on temperature (simple heat glow)
            if (temperature > 500.0f) {
                float intensity = std::min((temperature - 500.0f) / 1000.0f, 1.0f);
                uint8_t r = static_cast<uint8_t>(std::min(255.0f, ((finalColor >> 0) & 0xFF) + intensity * 100));
                uint8_t g = ((finalColor >> 8) & 0xFF);
                uint8_t b = ((finalColor >> 16) & 0xFF);
                uint8_t a = ((finalColor >> 24) & 0xFF);
                return (a << 24) | (b << 16) | (g << 8) | r;
            }
            
            return finalColor;
        }
    }
    
    // Default colors for basic materials
    switch (material) {
        case 1: return 0xFF8080C0; // Sand
        case 2: return 0xFFDF4020; // Water  
        case 3: return 0xFF0064FF; // Fire
        default: return 0xFF808080; // Unknown
    }
}

uint32_t SimulationWorld::ApplyVisualPattern(uint32_t baseColor, const VisualProperties& props, int x, int y) const {
    if (props.pattern == VisualPattern::Solid) {
        return baseColor;
    }
    
    // Extract RGBA components from base color
    uint8_t baseR = (baseColor >> 0) & 0xFF;
    uint8_t baseG = (baseColor >> 8) & 0xFF;
    uint8_t baseB = (baseColor >> 16) & 0xFF;
    uint8_t baseA = (baseColor >> 24) & 0xFF;
    
    // Generate secondary color as a subtle variation of the base color
    // Create both lighter and darker variants for different patterns
    auto generateVariant = [&](float factor) -> uint32_t {
        uint8_t varR = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, baseR * factor)));
        uint8_t varG = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, baseG * factor)));
        uint8_t varB = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, baseB * factor)));
        return (baseA << 24) | (varB << 16) | (varG << 8) | varR;
    };
    
    // Default secondary color (noticeably lighter)
    float variation = 1.5f; // 50% lighter for more visible patterns
    uint8_t secR = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, baseR * variation)));
    uint8_t secG = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, baseG * variation)));
    uint8_t secB = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, baseB * variation)));
    uint8_t secA = baseA;
    
    // Simple hash function for pseudo-random values
    auto simpleHash = [](int x, int y, int seed) -> uint32_t {
        uint32_t h = (x * 73856093) ^ (y * 19349663) ^ (seed * 83492791);
        h ^= h >> 16;
        h ^= h << 3;
        h ^= h >> 17;
        return h;
    };
    
    // Helper function to blend colors 
    auto blendColors = [&](float blend) -> uint32_t {
        uint8_t r = static_cast<uint8_t>(baseR * (1 - blend) + secR * blend);
        uint8_t g = static_cast<uint8_t>(baseG * (1 - blend) + secG * blend);
        uint8_t b = static_cast<uint8_t>(baseB * (1 - blend) + secB * blend);
        uint8_t a = static_cast<uint8_t>(baseA * (1 - blend) + secA * blend);
        return (a << 24) | (b << 16) | (g << 8) | r;
    };
    
    switch (props.pattern) {
        case VisualPattern::Speck: {
            uint32_t hash = simpleHash(x, y, 0);
            float speckChance = props.patternIntensity * 0.2f;
            return (hash & 0xFF) < (speckChance * 255) ? generateVariant(1.3f) : baseColor;
        }
        
        case VisualPattern::Wavy: {
            float phase = (x * props.patternScale * 0.3f) + (y * props.patternScale * 0.15f);
            float wave = sin(phase) * 0.5f + 0.5f;
            return blendColors(wave * props.patternIntensity);
        }
        
        case VisualPattern::Line: {
            int spacing = static_cast<int>(8.0f / props.patternScale);
            spacing = std::max(2, spacing);
            bool isLine = ((x % spacing) == 0) || ((y % spacing) == 0);
            return isLine ? generateVariant(0.7f) : baseColor; // Use darker variant for lines
        }
        
        case VisualPattern::Border: {
            uint32_t hash = simpleHash(x, y, 1);
            bool isBorder = (hash & 0x7) == 0;
            return isBorder ? blendColors(props.patternIntensity) : baseColor;
        }
        
        case VisualPattern::Gradient: {
            float gradient = (y * props.patternScale * 0.02f);
            gradient = fmod(gradient, 1.0f);
            return blendColors(gradient * props.patternIntensity);
        }
        
        case VisualPattern::Checkerboard: {
            int size = static_cast<int>(6.0f / props.patternScale);
            size = std::max(2, size);
            bool checker = ((x / size) + (y / size)) % 2;
            return checker ? generateVariant(0.6f) : baseColor; // Use darker variant for checkers
        }
        
        case VisualPattern::Dots: {
            int spacing = static_cast<int>(8.0f / props.patternScale);
            spacing = std::max(3, spacing);
            bool isDot = (x % spacing == spacing/2) && (y % spacing == spacing/2);
            return isDot ? generateVariant(1.4f) : baseColor; // Use lighter variant for dots
        }
        
        case VisualPattern::Stripes: {
            int spacing = static_cast<int>(6.0f / props.patternScale);
            spacing = std::max(2, spacing);
            bool isStripe = ((x + y) / spacing) % 2;
            return isStripe ? blendColors(props.patternIntensity) : baseColor;
        }
        
        case VisualPattern::Noise: {
            uint32_t hash = simpleHash(x, y, 2);
            float noise = (hash & 0xFF) / 255.0f;
            return blendColors(noise * props.patternIntensity);
        }
        
        case VisualPattern::Marble: {
            float vein1 = sin(x * 0.1f + y * 0.05f) * 0.5f + 0.5f;
            float vein2 = sin(x * 0.07f - y * 0.08f + 3.14f) * 0.5f + 0.5f;
            float marble = (vein1 + vein2) * 0.5f;
            return blendColors(marble * props.patternIntensity);
        }
        
        case VisualPattern::Crystal: {
            float crystal = abs(sin(x * 0.2f) * cos(y * 0.2f));
            return blendColors(crystal * props.patternIntensity);
        }
        
        case VisualPattern::Honeycomb: {
            // Simplified hexagonal pattern
            float hex = sin(x * 0.3f) + sin((x * 0.15f) + (y * 0.26f)) + sin(y * 0.3f);
            hex = (hex + 3.0f) / 6.0f; // Normalize
            return blendColors(hex * props.patternIntensity);
        }
        
        case VisualPattern::Spiral: {
            float angle = static_cast<float>(atan2(y - 256, x - 256));
            float radius = static_cast<float>(sqrt((x - 256) * (x - 256) + (y - 256) * (y - 256)));
            float spiral = sin(angle * 3.0f + radius * 0.1f) * 0.5f + 0.5f;
            return blendColors(spiral * props.patternIntensity);
        }
        
        case VisualPattern::Ripple: {
            float centerX = 256, centerY = 256;
            float dist = static_cast<float>(sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)));
            float ripple = sin(dist * 0.2f) * 0.5f + 0.5f;
            return blendColors(ripple * props.patternIntensity);
        }
        
        case VisualPattern::Flame: {
            float flame = sin(x * 0.1f + y * 0.3f) * cos(y * 0.1f) * 0.5f + 0.5f;
            flame = std::max(0.0f, flame - (y * 0.002f)); // Fade with height
            return blendColors(flame * props.patternIntensity);
        }
        
        case VisualPattern::Wood: {
            float rings = sin(static_cast<float>(sqrt(x * x + y * y)) * 0.1f) * 0.5f + 0.5f;
            float grain = sin(x * 0.05f + y * 0.02f) * 0.3f;
            return blendColors((rings + grain) * props.patternIntensity);
        }
        
        case VisualPattern::Metal: {
            float brush = sin(x * 0.2f + y * 0.05f) * 0.3f + 0.7f;
            uint32_t hash = simpleHash(x, y, 3);
            float scratch = (hash & 0x1F) / 31.0f * 0.2f;
            return blendColors((brush + scratch) * props.patternIntensity);
        }
        
        case VisualPattern::Fabric: {
            bool warp = (x % 4) < 2;
            bool weft = (y % 4) < 2;
            float weave = (warp == weft) ? 0.8f : 0.2f;
            return blendColors(weave * props.patternIntensity);
        }
        
        case VisualPattern::Scale: {
            int scaleSize = 6;
            int sx = x / scaleSize, sy = y / scaleSize;
            bool scale = ((sx + sy) % 2) && ((x % scaleSize) < scaleSize/2);
            return scale ? generateVariant(0.8f) : baseColor; // Use slightly darker variant for scales
        }
        
        case VisualPattern::Bubble: {
            uint32_t hash = simpleHash(x / 8, y / 8, 4);
            float bubble = (hash & 0x3F) / 63.0f;
            if (bubble > 0.7f) {
                int bx = x % 8, by = y % 8;
                float dist = static_cast<float>(sqrt((bx - 4) * (bx - 4) + (by - 4) * (by - 4)));
                return (dist < 3.0f) ? generateVariant(1.3f) : baseColor; // Use lighter variant for bubbles
            }
            return baseColor;
        }
        
        case VisualPattern::Crack: {
            uint32_t hash1 = simpleHash(x, y, 5);
            uint32_t hash2 = simpleHash(x + 1, y, 5);
            uint32_t hash3 = simpleHash(x, y + 1, 5);
            bool crack = (hash1 > hash2 && hash1 > hash3) && ((hash1 & 0xFF) > 240);
            return crack ? generateVariant(0.5f) : baseColor; // Use darker variant for cracks
        }
        
        case VisualPattern::Flow: {
            float flow = sin(x * 0.1f + y * 0.15f + (x + y) * 0.05f) * 0.5f + 0.5f;
            return blendColors(flow * props.patternIntensity);
        }
        
        case VisualPattern::Spark: {
            uint32_t hash = simpleHash(x, y, 6);
            bool spark = ((hash & 0xFF) > 250) && ((hash >> 8) & 0x3) == 0;
            return spark ? blendColors(1.0f) : baseColor;
        }
        
        case VisualPattern::Glow: {
            uint32_t hash = simpleHash(x / 4, y / 4, 7);
            bool glowCenter = (hash & 0x1F) > 28;
            if (glowCenter) {
                int gx = x % 4, gy = y % 4;
                float dist = static_cast<float>(sqrt((gx - 2) * (gx - 2) + (gy - 2) * (gy - 2)));
                float glow = std::max(0.0f, 1.0f - dist / 2.0f);
                return blendColors(glow * props.patternIntensity);
            }
            return baseColor;
        }
        
        case VisualPattern::Frost: {
            float frost1 = sin(x * 0.3f) * cos(y * 0.25f);
            float frost2 = sin(x * 0.15f + y * 0.2f);
            float frost = (frost1 + frost2) * 0.5f + 0.5f;
            uint32_t hash = simpleHash(x, y, 8);
            frost *= (hash & 0x7F) / 127.0f;
            return blendColors(frost * props.patternIntensity);
        }
        
        case VisualPattern::Sand: {
            uint32_t hash1 = simpleHash(x, y, 9);
            uint32_t hash2 = simpleHash(x + 7, y + 13, 9);
            float grain = ((hash1 & 0x7F) + (hash2 & 0x7F)) / 254.0f;
            return blendColors(grain * props.patternIntensity * 0.6f);
        }
        
        case VisualPattern::Rock: {
            uint32_t hash = simpleHash(x / 3, y / 3, 10);
            float rock = (hash & 0x3F) / 63.0f;
            rock += sin(x * 0.08f) * cos(y * 0.06f) * 0.3f;
            return blendColors(rock * props.patternIntensity);
        }
        
        case VisualPattern::Plasma: {
            float plasma = sin(x * 0.1f) + sin(y * 0.1f) + sin((x + y) * 0.1f) + sin(static_cast<float>(sqrt(x * x + y * y)) * 0.1f);
            plasma = (plasma + 4.0f) / 8.0f;
            return blendColors(plasma * props.patternIntensity);
        }
        
        case VisualPattern::Lightning: {
            uint32_t hash = simpleHash(x, y, 11);
            bool bolt = ((hash & 0xFF) > 253) && (abs(sin(x * 0.1f + y * 0.05f)) > 0.8f);
            return bolt ? blendColors(1.0f) : baseColor;
        }
        
        case VisualPattern::Smoke: {
            float wisp = sin(x * 0.05f + y * 0.1f) * cos(x * 0.08f) * 0.5f + 0.5f;
            wisp *= (1.0f - y * 0.002f); // Fade with height
            return blendColors(wisp * props.patternIntensity);
        }
        
        case VisualPattern::Steam: {
            uint32_t hash = simpleHash(x / 2, y / 2, 12);
            float steam = (hash & 0x3F) / 63.0f;
            steam *= sin(x * 0.1f + y * 0.2f) * 0.5f + 0.5f;
            return blendColors(steam * props.patternIntensity * 0.7f);
        }
        
        case VisualPattern::Oil: {
            // Subtle oil slick effect - just slight variations in darkness
            float slick = sin(x * 0.2f + y * 0.15f) * 0.3f + 0.7f;
            return blendColors(slick * props.patternIntensity * 0.4f);
        }
        
        case VisualPattern::Blood: {
            uint32_t hash = simpleHash(x / 5, y / 5, 13);
            bool droplet = (hash & 0x1F) > 28;
            if (droplet) {
                int dx = x % 5, dy = y % 5;
                float dist = static_cast<float>(sqrt((dx - 2.5f) * (dx - 2.5f) + (dy - 2.5f) * (dy - 2.5f)));
                return (dist < 2.0f) ? blendColors(props.patternIntensity) : baseColor;
            }
            return baseColor;
        }
        
        case VisualPattern::Acid: {
            float bubble = sin(x * 0.3f + y * 0.25f) * cos(x * 0.2f - y * 0.3f) * 0.5f + 0.5f;
            uint32_t hash = simpleHash(x, y, 14);
            bubble *= (hash & 0x7F) / 127.0f;
            return blendColors(bubble * props.patternIntensity);
        }
        
        case VisualPattern::Ice: {
            float crystal = abs(sin(x * 0.2f) * cos(y * 0.15f));
            float fractal = sin(x * 0.1f + y * 0.1f) * 0.3f;
            return blendColors((crystal + fractal) * props.patternIntensity);
        }
        
        case VisualPattern::Lava: {
            float flow = sin(x * 0.05f + y * 0.1f) * 0.5f + 0.5f;
            float heat = sin(x * 0.2f) * cos(y * 0.15f) * 0.3f + 0.7f;
            return blendColors((flow + heat) * props.patternIntensity * 0.5f);
        }
        
        case VisualPattern::Gas: {
            uint32_t hash = simpleHash(x / 3, y / 3, 15);
            float particle = (hash & 0x3F) / 63.0f;
            particle *= sin(x * 0.15f + y * 0.1f) * 0.5f + 0.5f;
            return blendColors(particle * props.patternIntensity * 0.4f);
        }
        
        case VisualPattern::Liquid: {
            float surface = sin(x * 0.1f + y * 0.05f) * 0.2f + 0.8f;
            float tension = cos(x * 0.2f - y * 0.1f) * 0.1f;
            return blendColors((surface + tension) * props.patternIntensity);
        }
        
        case VisualPattern::Powder: {
            uint32_t hash1 = simpleHash(x, y, 16);
            uint32_t hash2 = simpleHash(x + 3, y + 7, 16);
            float grain = ((hash1 & 0x1F) + (hash2 & 0x1F)) / 62.0f;
            return blendColors(grain * props.patternIntensity * 0.5f);
        }
        
        default:
            return baseColor;
    }
}

uint32_t SimulationWorld::BlendEffectLayer(uint32_t baseColor, EffectLayer effect, uint8_t intensity) const {
    if (intensity == 0) return baseColor;
    
    // Extract RGBA components from base color
    uint8_t baseR = (baseColor >> 0) & 0xFF;
    uint8_t baseG = (baseColor >> 8) & 0xFF;
    uint8_t baseB = (baseColor >> 16) & 0xFF;
    uint8_t baseA = (baseColor >> 24) & 0xFF;
    
    // Effect colors and blend modes
    uint8_t effectR, effectG, effectB;
    float blend = intensity / 255.0f;
    
    switch (effect) {
        case EffectLayer::Burning: {
            // Orange-red flickering glow
            effectR = 255;
            effectG = static_cast<uint8_t>(140 + (rand() % 60)); // Flicker between orange and red
            effectB = 0;
            break;
        }
        
        case EffectLayer::Freezing: {
            // Blue-white ice crystals
            effectR = static_cast<uint8_t>(200 + (rand() % 55));
            effectG = static_cast<uint8_t>(220 + (rand() % 35));
            effectB = 255;
            break;
        }
        
        case EffectLayer::Electrified: {
            // Blue-white electric sparks
            uint8_t spark = static_cast<uint8_t>(200 + (rand() % 56));
            effectR = spark;
            effectG = spark;
            effectB = 255;
            break;
        }
        
        case EffectLayer::Bloodied: {
            // Dark red blood stains
            effectR = static_cast<uint8_t>(150 + (rand() % 50));
            effectG = 20;
            effectB = 20;
            break;
        }
        
        case EffectLayer::Blackened: {
            // Soot and explosion damage
            uint8_t soot = static_cast<uint8_t>(30 + (rand() % 40));
            effectR = soot;
            effectG = soot;
            effectB = soot;
            blend *= 0.8f; // Darken existing color
            break;
        }
        
        case EffectLayer::Corroding: {
            // Green acid corrosion
            effectR = 50;
            effectG = static_cast<uint8_t>(200 + (rand() % 55));
            effectB = 50;
            break;
        }
        
        case EffectLayer::Crystallizing: {
            // Prismatic crystal formation
            float crystal = sin(static_cast<float>(rand() % 100) * 0.1f) * 0.5f + 0.5f;
            effectR = static_cast<uint8_t>(150 + crystal * 105);
            effectG = static_cast<uint8_t>(200 + crystal * 55);
            effectB = static_cast<uint8_t>(255);
            break;
        }
        
        case EffectLayer::Glowing: {
            // Bright luminescence
            effectR = 255;
            effectG = 255;
            effectB = static_cast<uint8_t>(200 + (rand() % 55));
            break;
        }
        
        default:
            return baseColor;
    }
    
    // Blend effect with base color
    uint8_t finalR = static_cast<uint8_t>(baseR * (1.0f - blend) + effectR * blend);
    uint8_t finalG = static_cast<uint8_t>(baseG * (1.0f - blend) + effectG * blend);
    uint8_t finalB = static_cast<uint8_t>(baseB * (1.0f - blend) + effectB * blend);
    
    return finalR | (finalG << 8) | (finalB << 16) | (baseA << 24);
}

} // namespace BGE