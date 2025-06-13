#include "SimulationWorld.h"
#include "Materials/MaterialSystem.h"
#include "World/ChunkManager.h"
#include "CellularAutomata.h"
#include "Physics/PhysicsWorld.h"
#include "../Core/Threading/ThreadPool.h"
#include <iostream>
#include <algorithm>
#include <cstring>
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

void SimulationWorld::UpdatePixelBuffer() {
    // Convert world state to pixel buffer
    static bool debugPrinted = false;
    uint32_t nonEmptyCount = 0;
    
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            const Cell& cell = GetCell(x, y);
            uint32_t color;
            
            // Get the actual material color
            color = MaterialToColor(cell.material, cell.temperature);
            
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

uint32_t SimulationWorld::MaterialToColor(MaterialID material, float temperature) const {
    if (material == MATERIAL_EMPTY) {
        return 0x00000000; // Transparent
    }
    
    // Get material color from material system
    if (m_materialSystem) {
        const Material* mat = m_materialSystem->GetMaterialPtr(material);
        if (mat) {
            uint32_t baseColor = mat->GetColor();
            
            // Debug: Print material colors once
            static std::set<MaterialID> printedMaterials;
            if (printedMaterials.find(material) == printedMaterials.end()) {
                std::cout << "Material " << material << " color: 0x" << std::hex << baseColor << std::dec << std::endl;
                printedMaterials.insert(material);
            }
            
            // Modify color based on temperature (simple heat glow)
            if (temperature > 500.0f) {
                float intensity = std::min((temperature - 500.0f) / 1000.0f, 1.0f);
                uint8_t r = static_cast<uint8_t>(std::min(255.0f, ((baseColor >> 0) & 0xFF) + intensity * 100));
                uint8_t g = ((baseColor >> 8) & 0xFF);
                uint8_t b = ((baseColor >> 16) & 0xFF);
                uint8_t a = ((baseColor >> 24) & 0xFF);
                return (a << 24) | (b << 16) | (g << 8) | r;
            }
            
            return baseColor;
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

} // namespace BGE