#include "GamePanel.h"
#include "../../Services.h"
#include "../../Logger.h"
#include "../../../Renderer/Renderer.h"
#include "../../Entity.h"
#include "../../Components.h"
#include "../../ECS/EntityManager.h"
#include "../../ECS/EntityQuery.h"
#include <imgui.h>
#include <chrono>
#include <cstdio>
#include <algorithm>

namespace BGE {

GamePanel::GamePanel(const std::string& name, SimulationWorld* world)
    : Panel(name, PanelDockPosition::Center)
    , m_world(world) {
}

void GamePanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void GamePanel::OnRender() {
    m_isFocused = ImGui::IsWindowFocused();
    m_isHovered = ImGui::IsWindowHovered();
    
    // Update playing state based on simulation
    m_isPlaying = !m_world->IsPaused();
    
    RenderGameToolbar();
    RenderGameContent();
}

void GamePanel::RenderGameToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    
    // Play/Pause control
    if (!m_isPlaying) {
        if (ImGui::Button(" Play Game ")) {
            m_world->Play();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start Playing (Space)");
    } else {
        if (ImGui::Button("Pause Game")) {
            m_world->Pause();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause Game (Space)");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        m_world->Stop();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop and Reset Game");
    
    // Separator
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    
    // Game speed control (only when playing)
    if (m_isPlaying) {
        static float gameSpeed = 1.0f;
        ImGui::SetNextItemWidth(80);
        if (ImGui::SliderFloat("##GameSpeed", &gameSpeed, 0.5f, 2.0f, "%.1fx")) {
            m_world->SetSimulationSpeed(gameSpeed);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Game Speed");
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
    }
    
    // Display options
    ImGui::Checkbox("Stats", &m_showStats);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Performance Statistics");
    
    ImGui::SameLine();
    ImGui::Checkbox("Fullscreen", &m_fullscreen);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fullscreen Game View");
    
    // Right-aligned game state indicator
    ImGui::SameLine();
    float rightOffset = ImGui::GetContentRegionAvail().x - 100;
    if (rightOffset > 0) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightOffset);
    }
    
    if (m_isPlaying) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "PLAYING");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
    }
    
    ImGui::PopStyleVar(2);
}

void GamePanel::RenderGameContent() {
    // Get available content region for the game
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Calculate viewport bounds in screen space
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    m_viewportX = cursorPos.x - viewport->WorkPos.x;
    m_viewportY = cursorPos.y - viewport->WorkPos.y;
    m_viewportWidth = contentRegion.x;
    m_viewportHeight = contentRegion.y;
    
    auto renderer = Services::GetRenderer();
    auto world = Services::GetWorld();
    
    if (renderer && world && m_viewportWidth > 0 && m_viewportHeight > 0) {
        // For now, use optimized direct rendering
        // We'll keep framebuffer support for future post-processing effects
        RenderOptimizedWorld();
        
        // Show stats if enabled
        if (m_showStats) {
            RenderStats();
        }
        
        // Add game-specific border
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 borderColor;
        if (m_isPlaying) {
            borderColor = IM_COL32(0, 255, 0, 100); // Green when playing
        } else {
            borderColor = IM_COL32(255, 150, 0, 100); // Orange when paused
        }
        drawList->AddRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), 
                         borderColor, 0.0f, 0, 2.0f);
        
        // Show game state overlay
        if (!m_isPlaying) {
            // Show pause overlay
            ImVec2 center = ImVec2(cursorPos.x + contentRegion.x * 0.5f, cursorPos.y + contentRegion.y * 0.5f);
            drawList->AddText(ImVec2(center.x - 30, center.y), 
                             IM_COL32(255, 255, 255, 200), "PAUSED");
            drawList->AddText(ImVec2(center.x - 50, center.y + 20), 
                             IM_COL32(200, 200, 200, 150), "Press Play to start");
        }
        
        // Show performance stats if enabled
        if (m_showStats && m_isPlaying) {
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), 
                             IM_COL32(255, 255, 0, 200), "FPS: 60"); // TODO: Get actual FPS
            drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 30), 
                             IM_COL32(255, 255, 0, 200), "Entities: 0"); // TODO: Get entity count
        }
        
        // Handle game input only when playing
        bool imageHovered = ImGui::IsItemHovered();
        if (imageHovered && m_isPlaying) {
            HandleGameInput(cursorPos, contentRegion);
        }
        
        // Handle pause/play with spacebar when focused
        if ((m_isFocused || imageHovered) && ImGui::IsKeyPressed(ImGuiKey_Space)) {
            if (m_isPlaying) {
                m_world->Pause();
            } else {
                m_world->Play();
            }
        }
        
    } else {
        // Fallback when no renderer available
        ImGui::InvisibleButton("GameViewport", contentRegion);
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 bgColor = IM_COL32(20, 20, 30, 255);
        drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), bgColor);
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 10), 
                         IM_COL32(255, 255, 255, 255), "Game Panel");
        drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y + 30), 
                         IM_COL32(200, 200, 200, 255), "Pure gameplay experience");
    }
}

void GamePanel::HandleGameInput(ImVec2 cursorPos, ImVec2 contentRegion) {
    // Only handle game input when actually playing
    if (!m_isPlaying) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    float relativeX = mousePos.x - cursorPos.x;
    float relativeY = mousePos.y - cursorPos.y;
    
    // Ensure mouse is within bounds
    if (relativeX < 0 || relativeY < 0 || relativeX >= contentRegion.x || relativeY >= contentRegion.y) {
        return;
    }
    
    // TODO: Handle actual game input (movement, actions, etc.)
    // This would depend on what kind of game mechanics we want to support
    
    // For now, just handle basic mouse interaction
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        BGE_LOG_INFO("GamePanel", "Game input: Left click at (" + 
                    std::to_string(relativeX) + ", " + std::to_string(relativeY) + ")");
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        BGE_LOG_INFO("GamePanel", "Game input: Right click at (" + 
                    std::to_string(relativeX) + ", " + std::to_string(relativeY) + ")");
    }
}

void GamePanel::RenderOptimizedWorld() {
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    auto world = Services::GetWorld();
    if (!world) return;
    
    // Create invisible button for input handling
    ImGui::InvisibleButton("GameViewport", contentRegion);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Set clip rect to prevent drawing outside the panel
    drawList->PushClipRect(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), true);
    
    // Draw dark background
    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + contentRegion.x, cursorPos.y + contentRegion.y), 
                           IM_COL32(30, 30, 40, 255));
    
    // Calculate visible world bounds based on viewport
    int startX, startY, endX, endY;
    CalculateVisibleBounds(startX, startY, endX, endY);
    
    // Calculate LOD step based on zoom level
    int step = CalculateLODStep();
    float blockSize = step * 1.0f; // 1:1 pixel mapping for game view
    
    // Render world pixels with LOD
    for (int y = startY; y < endY; y += step) {
        for (int x = startX; x < endX; x += step) {
            // Y-flip for correct orientation
            int flippedY = static_cast<int>(world->GetHeight()) - 1 - y;
            
            if (x >= 0 && x < static_cast<int>(world->GetWidth()) && flippedY >= 0 && flippedY < static_cast<int>(world->GetHeight())) {
                uint32_t pixel = world->GetPixelData()[flippedY * world->GetWidth() + x];
                if (pixel != 0) {
                    uint8_t r = (pixel >> 16) & 0xFF;
                    uint8_t g = (pixel >> 8) & 0xFF;
                    uint8_t b = pixel & 0xFF;
                    
                    // Calculate screen position
                    float screenX = cursorPos.x + x;
                    float screenY = cursorPos.y + y;
                    
                    drawList->AddRectFilled(
                        ImVec2(screenX, screenY),
                        ImVec2(screenX + blockSize, screenY + blockSize),
                        IM_COL32(r, g, b, 255)
                    );
                }
            }
        }
    }
    
    // Draw entities on top
    auto& entityManager = EntityManager::Instance();
    EntityQuery query(&entityManager);
    query.template With<TransformComponent>().template ForEach<TransformComponent>([&](EntityID id, TransformComponent& transform) {
        // Convert world position to screen position (game view is 1:1)
        float screenX = cursorPos.x + transform.position.x;
        float screenY = cursorPos.y + (static_cast<float>(world->GetHeight()) - transform.position.y); // Y-flip
        
        // Only render if visible
        if (screenX >= cursorPos.x - 10 && screenX <= cursorPos.x + contentRegion.x + 10 &&
            screenY >= cursorPos.y - 10 && screenY <= cursorPos.y + contentRegion.y + 10) {
            
            // Draw entity marker
            drawList->AddCircleFilled(ImVec2(screenX, screenY), 5.0f, IM_COL32(255, 255, 0, 200));
            
            // Draw entity name if available
            auto* name = entityManager.GetComponent<NameComponent>(id);
            if (name) {
                drawList->AddText(ImVec2(screenX + 8, screenY - 8), 
                                 IM_COL32(255, 255, 255, 200), 
                                 name->name.c_str());
            }
        }
    });
    
    drawList->PopClipRect();
}

void GamePanel::CalculateVisibleBounds(int& startX, int& startY, int& endX, int& endY) {
    auto world = Services::GetWorld();
    if (!world) {
        startX = startY = 0;
        endX = endY = 0;
        return;
    }
    
    // For game view, we show the entire viewport 1:1
    // Since we're rendering at pixel scale, visible bounds are simply the viewport size
    startX = 0;
    startY = 0;
    endX = std::min(static_cast<int>(m_viewportWidth), static_cast<int>(world->GetWidth()));
    endY = std::min(static_cast<int>(m_viewportHeight), static_cast<int>(world->GetHeight()));
}

int GamePanel::CalculateLODStep() const {
    // For game view, we want full detail most of the time
    // Only use LOD when viewport is significantly larger than world
    auto world = Services::GetWorld();
    if (!world) return 1;
    
    // If viewport is larger than world, we might want to use LOD
    float scaleX = m_viewportWidth / static_cast<float>(world->GetWidth());
    float scaleY = m_viewportHeight / static_cast<float>(world->GetHeight());
    float scale = std::min(scaleX, scaleY);
    
    if (scale > 2.0f) return 4;  // Very zoomed out
    if (scale > 1.5f) return 2;  // Moderately zoomed out
    return 1;  // Normal view - full detail
}

void GamePanel::RenderStats() {
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Get stats
    auto& entityManager = EntityManager::Instance();
    int entityCount = static_cast<int>(entityManager.GetEntityCount());
    
    // Calculate FPS (simple frame time based)
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count() / 1000000.0f;
    lastTime = currentTime;
    float fps = deltaTime > 0 ? 1.0f / deltaTime : 0.0f;
    
    // Draw stats overlay
    char statsText[256];
    snprintf(statsText, sizeof(statsText), "FPS: %.1f\nEntities: %d\nViewport: %.0fx%.0f", 
             fps, entityCount, m_viewportWidth, m_viewportHeight);
    
    drawList->AddText(ImVec2(cursorPos.x + 10, cursorPos.y - 80), 
                     IM_COL32(255, 255, 0, 200), 
                     statsText);
}

} // namespace BGE