#include "../../Core/Engine.h"
#include "../../Core/Application.h"
#include "../../Core/Services.h"
#include "../../Core/UI/Framework/UISystem.h"

// Example panels
#include "../../Core/UI/Panels/HierarchyPanel.h"
#include "../../Core/UI/Panels/InspectorPanel.h"
#include "../../Core/UI/Panels/SceneViewPanel.h"
#include "../../Core/UI/Panels/AssetBrowserPanel.h"
#include "../../Core/UI/Panels/MaterialPalettePanel.h"

using namespace BGE;

class DockingSystemDemo : public Application {
public:
    bool Initialize() override {
        // Get UI system
        auto* uiSystem = Services::GetUISystem();
        if (!uiSystem) {
            std::cerr << "Failed to get UISystem service" << std::endl;
            return false;
        }
        
        // Register panels with the new docking system
        uiSystem->RegisterPanel<HierarchyPanel>("Hierarchy");
        uiSystem->RegisterPanel<InspectorPanel>("Inspector");
        uiSystem->RegisterPanel<SceneViewPanel>("Scene");
        uiSystem->RegisterPanel<AssetBrowserPanel>("Assets");
        uiSystem->RegisterPanel<MaterialPalettePanel>("Materials");
        
        std::cout << "=== Unity-Style Docking System Demo ===" << std::endl;
        std::cout << "Instructions:" << std::endl;
        std::cout << "1. Drag any panel tab to move it" << std::endl;
        std::cout << "2. Drop on screen edges to split areas" << std::endl;
        std::cout << "3. Drop on center to create tabs" << std::endl;
        std::cout << "4. Drag away to create floating windows" << std::endl;
        std::cout << "5. Drag splitter handles to resize areas" << std::endl;
        std::cout << "===========================================" << std::endl;
        
        return true;
    }
    
    void Update(float deltaTime) override {
        // Main application update
    }
    
    void Render() override {
        auto* uiSystem = Services::GetUISystem();
        if (uiSystem && uiSystem->IsEnabled()) {
            uiSystem->BeginFrame();
            
            // Create main menu bar
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("Window")) {
                    if (ImGui::MenuItem("Reset Layout")) {
                        uiSystem->GetDockingSystem().ResetToDefaultLayout();
                    }
                    ImGui::Separator();
                    
                    // Toggle panels
                    if (ImGui::MenuItem("Hierarchy")) {
                        uiSystem->GetDockingSystem().TogglePanel("Hierarchy");
                    }
                    if (ImGui::MenuItem("Inspector")) {
                        uiSystem->GetDockingSystem().TogglePanel("Inspector");
                    }
                    if (ImGui::MenuItem("Scene")) {
                        uiSystem->GetDockingSystem().TogglePanel("Scene");
                    }
                    if (ImGui::MenuItem("Materials")) {
                        uiSystem->GetDockingSystem().TogglePanel("Materials");
                    }
                    
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
            
            // Render the docking system (replaces old BeginDockspace/EndDockspace)
            uiSystem->BeginDockspace();
            
            uiSystem->EndFrame();
        }
    }
};

int main() {
    DockingSystemDemo app;
    return app.Run();
}