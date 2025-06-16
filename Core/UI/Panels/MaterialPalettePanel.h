#pragma once

#include "../Framework/Panel.h"
#include "../../Input/MaterialTools.h"

namespace BGE {

class MaterialPalettePanel : public Panel {
public:
    MaterialPalettePanel(const std::string& name, MaterialTools* tools);
    
    void Initialize() override;
    void OnRender() override;
    
private:
    void RenderMaterialGrid();
    void RenderSelectedMaterialInfo();
    void RenderBrushSettings();
    
    MaterialTools* m_tools;
    int m_gridColumns = 8;
    float m_materialButtonSize = 40.0f;
};

} // namespace BGE