#include "HierarchyPanel.h"
#include "../Services.h"
#include "../../Core/Components.h"
#include "../../Core/Entity.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/PixelCamera.h"
#include <imgui.h>

namespace BGE {

HierarchyPanel::HierarchyPanel(const std::string& name, SimulationWorld* world)
    : Panel(name, PanelDockPosition::Left)
    , m_world(world) {
}

void HierarchyPanel::Initialize() {
    SetWindowFlags(ImGuiWindowFlags_NoCollapse);
}

void HierarchyPanel::OnRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    
    RenderEntityHierarchy();
    
    ImGui::PopStyleVar();
}

void HierarchyPanel::RenderEntityHierarchy() {
    // Camera Section
    if (ImGui::CollapsingHeader("📷 Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderCameraSection();
    }
    
    // Lights Section  
    if (ImGui::CollapsingHeader("💡 Lights")) {
        RenderLightsSection();
    }
    
    // Rigid Bodies Section
    if (ImGui::CollapsingHeader("🟦 Rigid Bodies")) {
        RenderRigidBodiesSection();
    }
    
    // Other Entities Section
    if (ImGui::CollapsingHeader("📦 Other Entities")) {
        RenderOtherEntitiesSection();
    }
}

void HierarchyPanel::RenderCameraSection() {
    // Get renderer for camera info
    auto renderer = Services::GetRenderer();
    if (renderer && renderer->GetPixelCamera()) {
        auto* camera = renderer->GetPixelCamera();
        
        ImGui::Indent();
        
        // Camera info
        Vector2 cameraPos = camera->GetPosition();
        int zoom = camera->GetZoom();
        
        if (ImGui::TreeNode("Main Camera")) {
            ImGui::Text("Position: (%.1f, %.1f)", cameraPos.x, cameraPos.y);
            ImGui::Text("Zoom: %dx", zoom);
            
            // Quick camera controls
            if (ImGui::Button("Center", ImVec2(-1, 20))) {
                camera->SetPosition(Vector2{0, 0});
            }
            
            ImGui::TreePop();
        }
        
        ImGui::Unindent();
    } else {
        ImGui::Indent();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No camera available");
        ImGui::Unindent();
    }
}

void HierarchyPanel::RenderLightsSection() {
    ImGui::Indent();
    
    // For now, show placeholder since we don't have a dedicated lighting system yet
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No lights in scene");
    
    // Future: Iterate through entities with LightComponent
    
    ImGui::Unindent();
}

void HierarchyPanel::RenderRigidBodiesSection() {
    ImGui::Indent();
    
    // Get entities with physics components
    auto& entityManager = EntityManager::Instance();
    bool foundRigidBodies = false;
    
    for (const auto& [id, entity] : entityManager.GetAllEntities()) {
        // Check if entity has physics-related components
        auto* velocity = entity->GetComponent<VelocityComponent>();
        
        if (velocity) { // Has velocity = is a rigid body
            foundRigidBodies = true;
            RenderEntity(entity.get(), "RigidBody");
        }
    }
    
    if (!foundRigidBodies) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No rigid bodies in scene");
    }
    
    ImGui::Unindent();
}

void HierarchyPanel::RenderOtherEntitiesSection() {
    ImGui::Indent();
    
    // Get all other entities (not camera, lights, or rigid bodies)
    auto& entityManager = EntityManager::Instance();
    bool foundOthers = false;
    
    for (const auto& [id, entity] : entityManager.GetAllEntities()) {
        auto* velocity = entity->GetComponent<VelocityComponent>();
        auto* name = entity->GetComponent<NameComponent>();
        
        // Skip rigid bodies (handled above)
        if (velocity) continue;
        
        // Skip camera entities (handled above)  
        if (name && (name->name.find("Camera") != std::string::npos)) continue;
        
        foundOthers = true;
        RenderEntity(entity.get(), "Entity");
    }
    
    if (!foundOthers) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No other entities in scene");
    }
    
    ImGui::Unindent();
}

void HierarchyPanel::RenderEntity(Entity* entity, const std::string& category) {
    if (!entity) return;
    
    auto* name = entity->GetComponent<NameComponent>();
    auto* transform = entity->GetComponent<TransformComponent>();
    auto* material = entity->GetComponent<MaterialComponent>();
    
    std::string entityName = name ? name->name : ("Entity_" + std::to_string(entity->GetID()));
    
    // Entity selection
    bool isSelected = (m_selectedEntity == entity);
    if (ImGui::Selectable(entityName.c_str(), isSelected)) {
        m_selectedEntity = entity;
    }
    
    // Show entity details in tooltip
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("ID: %llu", entity->GetID());
        ImGui::Text("Type: %s", category.c_str());
        
        if (transform) {
            ImGui::Text("Position: (%.1f, %.1f, %.1f)", 
                       transform->position.x, transform->position.y, transform->position.z);
        }
        
        if (material) {
            ImGui::Text("Material ID: %d", material->materialID);
        }
        
        auto* velocity = entity->GetComponent<VelocityComponent>();
        if (velocity) {
            ImGui::Text("Velocity: (%.1f, %.1f, %.1f)",
                       velocity->velocity.x, velocity->velocity.y, velocity->velocity.z);
        }
        
        ImGui::EndTooltip();
    }
}

} // namespace BGE