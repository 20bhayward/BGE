#pragma once

#include "ServiceLocator.h"

namespace BGE {

// Forward declarations
class Renderer;
class SimulationWorld;
class AudioSystem;
class InputManager;
class AssetManager;
class UISystem;
class ParticleSystem; // Forward declare ParticleSystem
class ProjectSettingsPanel;

namespace Services {
    template<typename T>
    inline std::shared_ptr<T> Get() {
        return ServiceLocator::Instance().GetService<T>();
    }
    
    // These functions will be defined in Services.cpp to avoid template instantiation issues
    std::shared_ptr<Renderer> GetRenderer();
    std::shared_ptr<SimulationWorld> GetWorld();
    std::shared_ptr<AudioSystem> GetAudio();
    std::shared_ptr<InputManager> GetInput();
    std::shared_ptr<AssetManager> GetAssets();
    std::shared_ptr<UISystem> GetUI();
    std::shared_ptr<ParticleSystem> GetParticles(); // Add getter for ParticleSystem
    ProjectSettingsPanel* GetProjectSettings();
    void SetProjectSettings(ProjectSettingsPanel* settings);
}

} // namespace BGE