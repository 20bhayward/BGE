#pragma once

#include "Panel.h"
#include "../Input/MaterialTools.h"
#include "../Input/CategorizedMaterialPalette.h"
#include "../../Simulation/Materials/MaterialSystem.h"
#include <vector>
#include <string>

namespace BGE {

class AssetBrowserPanel : public Panel {
public:
    AssetBrowserPanel(const std::string& name, MaterialTools* materialTools);
    
    void Initialize() override;
    void OnRender() override;
    
private:
    void RenderMaterialsTab();
    void RenderTexturesTab();
    void RenderPowersTab();
    void RenderMaterialGrid(const std::vector<PaletteMaterial>& materials);
    
    MaterialTools* m_materialTools;
    int m_selectedTab = 0;
    float m_thumbnailSize = 64.0f;
    int m_columns = 4;
    
    // Search and filter
    char m_searchBuffer[256] = "";
    MaterialCategory m_filterCategory = MaterialCategory::All;
};

} // namespace BGE