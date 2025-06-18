#pragma once

#include "../Framework/Panel.h"
#include "../../ECS/EntityManager.h"
#include "../../ECS/ArchetypeManager.h"
#include "../../ECS/ComponentRegistry.h"
#include <vector>
#include <string>

namespace BGE {

class ArchetypeDebuggerPanel : public Panel {
public:
    ArchetypeDebuggerPanel(const std::string& name);
    ~ArchetypeDebuggerPanel() = default;
    
    void Initialize() override;
    void OnRender() override;
    
private:
    // Rendering sections
    void RenderOverview();
    void RenderArchetypeList();
    void RenderArchetypeDetails(uint32_t archetypeIndex);
    void RenderEntityList(Archetype* archetype);
    void RenderPerformanceStats();
    void RenderComponentRegistry();
    
    // Helper functions
    std::string GetArchetypeDescription(Archetype* archetype) const;
    std::string FormatComponentMask(const ComponentMask& mask) const;
    const char* GetComponentIcon(ComponentTypeID typeId) const;
    
    // State
    uint32_t m_selectedArchetype = UINT32_MAX;
    bool m_showEntityList = false;
    bool m_showComponentBits = false;
    bool m_showPerformanceStats = true;
    
    // Performance tracking
    struct PerformanceStats {
        size_t totalEntities = 0;
        size_t totalArchetypes = 0;
        size_t totalComponents = 0;
        size_t memoryUsageBytes = 0;
        float lastQueryTimeMs = 0.0f;
    };
    PerformanceStats m_stats;
    
    // Update performance stats
    void UpdateStats();
};

} // namespace BGE