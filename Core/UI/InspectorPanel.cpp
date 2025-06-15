#include "InspectorPanel.h"
#include "../Services.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/PostProcessor.h"
#include "../../Renderer/ParticleSystem.h"
#include "../../Renderer/PixelCamera.h"
#include <imgui.h>

namespace BGE {

InspectorPanel::InspectorPanel(const std::string& name, MaterialTools* tools, SimulationWorld* world)
    : Panel(name, PanelDockPosition::Right)
    , m_tools(tools)
    , m_world(world) {
}

void InspectorPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
}

void InspectorPanel::OnRender() {
    if (ImGui::BeginTabBar("InspectorTabs")) {
        if (ImGui::BeginTabItem("Material")) {
            RenderMaterialInfo();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Simulation")) {
            RenderSimulationInfo();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Camera & Effects")) {
            RenderCameraAndEffects();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}

void InspectorPanel::RenderMaterialInfo() {
    const MaterialPalette& palette = m_tools->GetPalette();
    const PaletteMaterial* selectedMat = palette.GetMaterial(palette.GetSelectedIndex());
    
    if (selectedMat) {
        ImGui::Text("Selected Material");
        ImGui::Separator();
        
        ImGui::Text("Name: %s", selectedMat->name.c_str());
        ImGui::Text("ID: %d", static_cast<int>(selectedMat->id));
        
        // Color display
        float r = ((selectedMat->color >> 0) & 0xFF) / 255.0f;
        float g = ((selectedMat->color >> 8) & 0xFF) / 255.0f;
        float b = ((selectedMat->color >> 16) & 0xFF) / 255.0f;
        float a = ((selectedMat->color >> 24) & 0xFF) / 255.0f;
        
        ImGui::ColorButton("Material Color", ImVec4(r, g, b, a), 
                          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                          ImVec2(50, 50));
        
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Color: #%08X", selectedMat->color);
        if (selectedMat->hotkey != -1) {
            ImGui::Text("Hotkey: %c", static_cast<char>(selectedMat->hotkey));
        }
        ImGui::EndGroup();
        
        ImGui::Separator();
        ImGui::TextWrapped("Description:");
        ImGui::TextWrapped("%s", selectedMat->description.c_str());
        
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No material selected");
    }
    
    ImGui::Spacing();
    
    // Material inspector toggle
    bool inspectorEnabled = m_tools->IsInspectorEnabled();
    if (ImGui::Checkbox("Enable Live Inspector", &inspectorEnabled)) {
        m_tools->SetInspectorEnabled(inspectorEnabled);
    }
    
    if (inspectorEnabled) {
        ImGui::Separator();
        const auto& materialInfo = m_tools->GetInspectedMaterial();
        
        if (materialInfo.hasData) {
            ImGui::Text("Inspected Material");
            ImGui::Text("Position: (%d, %d)", materialInfo.posX, materialInfo.posY);
            ImGui::Text("Temperature: %.1f°C", materialInfo.temperature);
            ImGui::Text("Density: %.2f", materialInfo.density);
            
            if (materialInfo.viscosity > 0.0f) {
                ImGui::Text("Viscosity: %.2f", materialInfo.viscosity);
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Hover over materials to inspect");
        }
    }
}


void InspectorPanel::RenderSimulationInfo() {
    // Compact status indicators
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    
    // Status with colored indicator
    bool isPaused = m_world->IsPaused();
    ImVec4 statusColor = isPaused ? ImVec4(0.8f, 0.4f, 0.2f, 1.0f) : ImVec4(0.2f, 0.8f, 0.4f, 1.0f);
    ImGui::TextColored(statusColor, "● %s", isPaused ? "PAUSED" : "RUNNING");
    
    // Compact info grid
    ImGui::Text("F:%lld  C:%d  T:%.1fms", 
                static_cast<long long>(m_world->GetUpdateCount()),
                m_world->GetActiveCells(), 
                m_world->GetLastUpdateTime() * 1000.0f);
    
    ImGui::Text("World: %dx%d", m_world->GetWidth(), m_world->GetHeight());
    
    // Compact speed control
    static float simSpeed = 1.0f;
    ImGui::SetNextItemWidth(-40);
    if (ImGui::SliderFloat("##Speed", &simSpeed, 0.1f, 3.0f, "%.1fx")) {
        m_world->SetSimulationSpeed(simSpeed);
    }
    ImGui::SameLine();
    if (ImGui::Button("1x", ImVec2(35, 0))) {
        simSpeed = 1.0f;
        m_world->SetSimulationSpeed(simSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset Speed");
    
    // Compact action buttons
    if (ImGui::Button("Clear", ImVec2(-1, 20))) {
        m_world->Clear();
    }
    if (ImGui::Button("Reset", ImVec2(-1, 20))) {
        m_world->Reset();
    }
    
    ImGui::PopStyleVar();
}

void InspectorPanel::RenderCameraAndEffects() {
    // Get services
    auto renderer = Services::GetRenderer();
    if (!renderer) {
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "⚠ Renderer unavailable");
        return;
    }
    
    auto postProcessor = renderer->GetPostProcessor();
    auto pixelCamera = renderer->GetPixelCamera();
    auto particleSystem = Services::GetParticles();
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 3));
    
    // Compact Camera Controls
    if (ImGui::CollapsingHeader("📷 Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (pixelCamera) {
            Vector2 cameraPos = pixelCamera->GetPosition();
            float pos[2] = {cameraPos.x, cameraPos.y};
            
            ImGui::SetNextItemWidth(-50);
            if (ImGui::SliderFloat2("##Pos", pos, -500.0f, 500.0f)) {
                pixelCamera->SetPosition(Vector2{pos[0], pos[1]});
            }
            ImGui::SameLine();
            if (ImGui::Button("⌂", ImVec2(45, 0))) {
                pixelCamera->SetPosition(Vector2{0, 0});
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Center");
            
            int zoom = pixelCamera->GetZoom();
            ImGui::SetNextItemWidth(-50);
            if (ImGui::SliderInt("##Zoom", &zoom, 1, 10, "%dx")) {
                pixelCamera->SetZoom(zoom);
            }
            ImGui::SameLine();
            if (ImGui::Button("1x", ImVec2(45, 0))) {
                pixelCamera->SetZoom(1);
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Camera unavailable");
        }
    }
    
    // Compact Effects Section
    if (ImGui::CollapsingHeader("✨ Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (postProcessor) {
            // Compact effect toggles in grid
            const char* effectIcons[] = { "💫", "🎨", "📺", "🟩" };
            const char* effectNames[] = { "Bloom", "Color", "Retro", "Pixel" };
            PostProcessEffect effects[] = { 
                PostProcessEffect::Bloom, 
                PostProcessEffect::ColorGrading, 
                PostProcessEffect::Scanlines, 
                PostProcessEffect::Pixelation 
            };
            
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
            
            for (int i = 0; i < 4; i++) {
                if (i > 0 && i % 2 != 0) ImGui::SameLine();
                
                bool enabled = postProcessor->IsEffectEnabled(effects[i]);
                if (enabled) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                }
                
                if (ImGui::Button(effectIcons[i], ImVec2(40, 25))) {
                    if (enabled) {
                        postProcessor->DisableEffect(effects[i]);
                    } else {
                        postProcessor->EnableEffect(effects[i]);
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", effectNames[i]);
                }
                
                if (enabled) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::PopStyleVar(2);
            
            // Quick effect adjustment (only show if any effect is enabled)
            bool anyEffectEnabled = postProcessor->IsEffectEnabled(PostProcessEffect::Bloom) ||
                                   postProcessor->IsEffectEnabled(PostProcessEffect::ColorGrading);
            
            if (anyEffectEnabled) {
                ImGui::Separator();
                
                // Bloom quick settings
                if (postProcessor->IsEffectEnabled(PostProcessEffect::Bloom)) {
                    BloomConfig bloomConfig = postProcessor->GetBloomConfig();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::SliderFloat("##BloomIntensity", &bloomConfig.intensity, 0.5f, 3.0f, "💫%.1f")) {
                        postProcessor->SetBloomConfig(bloomConfig);
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bloom Intensity");
                }
                
                // Color grading quick settings
                if (postProcessor->IsEffectEnabled(PostProcessEffect::ColorGrading)) {
                    ColorGradingConfig colorConfig = postProcessor->GetColorGradingConfig();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::SliderFloat("##Contrast", &colorConfig.contrast, 0.5f, 2.0f, "🎨%.1f")) {
                        postProcessor->SetColorGradingConfig(colorConfig);
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Contrast");
                }
            }
            
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Effects unavailable");
        }
    }
    
    // Compact Particles Section
    if (ImGui::CollapsingHeader("💥 Particles")) {
        if (particleSystem) {
            size_t activeParticles = particleSystem->GetActiveParticleCount();
            size_t maxParticles = particleSystem->GetMaxParticles();
            
            // Compact particle info
            ImGui::Text("%zu/%zu", activeParticles, maxParticles);
            ImGui::SameLine();
            ImGui::ProgressBar(static_cast<float>(activeParticles) / maxParticles, ImVec2(-1, 12));
            
            // Compact gravity control
            static float gravityValue = 50.0f;
            static bool gravityInitialized = false;
            if (!gravityInitialized) {
                gravityValue = particleSystem->GetGravity();
                gravityInitialized = true;
            }
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##Gravity", &gravityValue, 0.0f, 200.0f, "G:%.0f")) {
                particleSystem->SetGravity(gravityValue);
            }
            
            // Compact action buttons
            if (ImGui::Button("✨ Sparks", ImVec2(-1, 20))) {
                particleSystem->CreateSparks(Vector2(256, 256), 25);
            }
            
            if (ImGui::Button("💥 Boom", ImVec2(-1, 20))) {
                particleSystem->CreateExplosion(Vector2(256, 256), 150.0f, 60);
                if (postProcessor) {
                    postProcessor->TriggerScreenShake(8.0f, 2.0f);
                }
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Particles unavailable");
        }
    }
    
    ImGui::PopStyleVar();
}

} // namespace BGE