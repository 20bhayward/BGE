#pragma once

#include "../Framework/Panel.h"

namespace BGE {

class MaterialTools;

class MaterialEditorPanel : public Panel {
public:
    MaterialEditorPanel(const std::string& name, MaterialTools* tools);
    
    void Initialize() override;
    void OnRender() override;
    
private:
    MaterialTools* m_materialTools;
    int m_selectedMaterial = 0;
    float m_brushSize = 5.0f;
    float m_brushStrength = 1.0f;
};

} // namespace BGE