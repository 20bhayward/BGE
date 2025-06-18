#include "ArchetypeDebuggerPanel.h"
#include "../../Logger.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>

namespace BGE {

ArchetypeDebuggerPanel::ArchetypeDebuggerPanel(const std::string& name)
    : Panel(name) {
}

void ArchetypeDebuggerPanel::Initialize() {
    UpdateStats();
}

void ArchetypeDebuggerPanel::OnRender() {
    if (!IsVisible()) return;
    
    ImGui::Begin(m_name.c_str(), &m_visible);
    
    UpdateStats();
    
    RenderOverview();
    ImGui::Separator();
    
    if (m_showPerformanceStats) {
        RenderPerformanceStats();
        ImGui::Separator();
    }
    
    if (ImGui::CollapsingHeader("Archetypes", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderArchetypeList();
    }
    
    if (ImGui::CollapsingHeader("Component Registry")) {
        RenderComponentRegistry();
    }
    
    ImGui::End();
}

void ArchetypeDebuggerPanel::RenderOverview() {
    ImGui::Text("ECS Archetype Debugger");
    ImGui::TextDisabled("Monitor and debug the Entity Component System");
    
    ImGui::Spacing();
    
    // Quick stats
    ImGui::Columns(3, nullptr, false);
    
    ImGui::Text("Total Entities");
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%zu", m_stats.totalEntities);
    
    ImGui::NextColumn();
    
    ImGui::Text("Total Archetypes");
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.8f, 1.0f), "%zu", m_stats.totalArchetypes);
    
    ImGui::NextColumn();
    
    ImGui::Text("Component Types");
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "%zu", m_stats.totalComponents);
    
    ImGui::Columns(1);
    
    ImGui::Spacing();
    
    // Options
    ImGui::Checkbox("Show Performance Stats", &m_showPerformanceStats);
    ImGui::SameLine();
    ImGui::Checkbox("Show Component Bits", &m_showComponentBits);
}

void ArchetypeDebuggerPanel::RenderArchetypeList() {
    auto& entityManager = EntityManager::Instance();
    auto& archetypeManager = entityManager.GetArchetypeManager();
    const auto& archetypes = archetypeManager.GetAllArchetypes();
    
    // Table of archetypes
    if (ImGui::BeginTable("ArchetypeTable", 4, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Entities", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Components", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();
        
        for (size_t i = 0; i < archetypes.size(); ++i) {
            Archetype* archetype = archetypes[i].get();
            if (!archetype) continue;
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            
            // ID
            bool isSelected = (m_selectedArchetype == i);
            if (ImGui::Selectable(std::to_string(i).c_str(), isSelected, 
                ImGuiSelectableFlags_SpanAllColumns)) {
                m_selectedArchetype = static_cast<uint32_t>(i);
            }
            
            // Entities
            ImGui::TableNextColumn();
            ImGui::Text("%zu", archetype->GetEntityCount());
            
            // Components
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetArchetypeDescription(archetype).c_str());
            
            // Memory estimate
            ImGui::TableNextColumn();
            size_t memoryKB = (archetype->GetEntityCount() * 64) / 1024; // Rough estimate
            ImGui::Text("%zu KB", memoryKB);
        }
        
        ImGui::EndTable();
    }
    
    // Selected archetype details
    if (m_selectedArchetype < archetypes.size()) {
        ImGui::Spacing();
        RenderArchetypeDetails(m_selectedArchetype);
    }
}

void ArchetypeDebuggerPanel::RenderArchetypeDetails(uint32_t archetypeIndex) {
    auto& entityManager = EntityManager::Instance();
    auto& archetypeManager = entityManager.GetArchetypeManager();
    Archetype* archetype = archetypeManager.GetArchetype(archetypeIndex);
    
    if (!archetype) return;
    
    ImGui::Text("Archetype %u Details", archetypeIndex);
    ImGui::Separator();
    
    // Component mask
    if (m_showComponentBits) {
        ImGui::Text("Component Mask: %s", FormatComponentMask(archetype->GetMask()).c_str());
    }
    
    // Component list
    ImGui::Text("Components:");
    auto& registry = ComponentRegistry::Instance();
    for (ComponentTypeID typeId : archetype->GetComponentTypes()) {
        auto* info = registry.GetComponentInfo(typeId);
        if (info) {
            ImGui::BulletText("%s (ID: %u, Size: %zu bytes)", 
                info->name.c_str(), typeId, info->size);
        }
    }
    
    ImGui::Spacing();
    
    // Entity list toggle
    if (ImGui::Button("Show Entity List")) {
        m_showEntityList = !m_showEntityList;
    }
    
    if (m_showEntityList) {
        RenderEntityList(archetype);
    }
}

void ArchetypeDebuggerPanel::RenderEntityList(Archetype* archetype) {
    const auto& entities = archetype->GetEntities();
    
    ImGui::Text("Entities in Archetype (%zu):", entities.size());
    
    // Limit display for performance
    size_t displayCount = std::min(entities.size(), size_t(100));
    
    if (ImGui::BeginChild("EntityList", ImVec2(0, 200), true)) {
        for (size_t i = 0; i < displayCount; ++i) {
            EntityID entity = entities[i];
            std::string name = EntityManager::Instance().GetEntityName(entity);
            if (name.empty()) {
                name = "Entity_" + std::to_string(entity.GetIndex());
            }
            
            ImGui::Text("%s (ID: %u, Gen: %u)", 
                name.c_str(), entity.GetIndex(), entity.GetGeneration());
        }
        
        if (displayCount < entities.size()) {
            ImGui::TextDisabled("... and %zu more", entities.size() - displayCount);
        }
    }
    ImGui::EndChild();
}

void ArchetypeDebuggerPanel::RenderPerformanceStats() {
    ImGui::Text("Performance Statistics");
    ImGui::Separator();
    
    // Memory usage
    float memoryMB = m_stats.memoryUsageBytes / (1024.0f * 1024.0f);
    ImGui::Text("Estimated Memory Usage: %.2f MB", memoryMB);
    
    // Entity density
    float avgEntitiesPerArchetype = m_stats.totalArchetypes > 0 ? 
        (float)m_stats.totalEntities / m_stats.totalArchetypes : 0.0f;
    ImGui::Text("Average Entities per Archetype: %.1f", avgEntitiesPerArchetype);
    
    // Component stats
    auto& registry = ComponentRegistry::Instance();
    ImGui::Text("Registered Component Types: %zu", registry.GetComponentCount());
    
    // Performance metrics placeholder
    ImGui::TextDisabled("Query Performance: TODO");
    ImGui::TextDisabled("System Update Time: TODO");
}

void ArchetypeDebuggerPanel::RenderComponentRegistry() {
    auto& registry = ComponentRegistry::Instance();
    const auto& components = registry.GetAllComponents();
    
    if (ImGui::BeginTable("ComponentTable", 4, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Alignment", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();
        
        for (const auto& [typeId, info] : components) {
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::Text("%u", typeId);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", info.name.c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%zu bytes", info.size);
            
            ImGui::TableNextColumn();
            ImGui::Text("%zu", info.alignment);
        }
        
        ImGui::EndTable();
    }
}

std::string ArchetypeDebuggerPanel::GetArchetypeDescription(Archetype* archetype) const {
    auto& registry = ComponentRegistry::Instance();
    const auto& types = archetype->GetComponentTypes();
    
    if (types.empty()) {
        return "[Empty]";
    }
    
    std::stringstream ss;
    bool first = true;
    
    for (ComponentTypeID typeId : types) {
        if (!first) ss << ", ";
        first = false;
        
        auto* info = registry.GetComponentInfo(typeId);
        if (info) {
            // Remove "Component" suffix for brevity
            std::string name = info->name;
            size_t pos = name.find("Component");
            if (pos != std::string::npos) {
                name = name.substr(0, pos);
            }
            ss << name;
        } else {
            ss << "Unknown(" << typeId << ")";
        }
    }
    
    return ss.str();
}

std::string ArchetypeDebuggerPanel::FormatComponentMask(const ComponentMask& mask) const {
    std::stringstream ss;
    ss << "0b";
    
    bool foundFirst = false;
    for (int i = MAX_COMPONENTS - 1; i >= 0; --i) {
        if (mask.test(i)) {
            foundFirst = true;
        }
        
        if (foundFirst) {
            ss << (mask.test(i) ? '1' : '0');
        }
    }
    
    if (!foundFirst) {
        ss << "0";
    }
    
    return ss.str();
}

const char* ArchetypeDebuggerPanel::GetComponentIcon(ComponentTypeID /*typeId*/) const {
    // TODO: Return appropriate icons for component types
    return nullptr;
}

void ArchetypeDebuggerPanel::UpdateStats() {
    auto& entityManager = EntityManager::Instance();
    auto& archetypeManager = entityManager.GetArchetypeManager();
    auto& registry = ComponentRegistry::Instance();
    
    m_stats.totalEntities = entityManager.GetEntityCount();
    m_stats.totalArchetypes = archetypeManager.GetAllArchetypes().size();
    m_stats.totalComponents = registry.GetComponentCount();
    
    // Estimate memory usage
    m_stats.memoryUsageBytes = 0;
    
    // Entity storage
    m_stats.memoryUsageBytes += m_stats.totalEntities * sizeof(EntityRecord);
    
    // Component storage (rough estimate)
    for (const auto& archetype : archetypeManager.GetAllArchetypes()) {
        if (archetype) {
            size_t entitiesInArchetype = archetype->GetEntityCount();
            size_t bytesPerEntity = 0;
            
            for (ComponentTypeID typeId : archetype->GetComponentTypes()) {
                auto* info = registry.GetComponentInfo(typeId);
                if (info) {
                    bytesPerEntity += info->size;
                }
            }
            
            m_stats.memoryUsageBytes += entitiesInArchetype * bytesPerEntity;
        }
    }
}

} // namespace BGE