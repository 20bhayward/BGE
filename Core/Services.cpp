#include "Services.h"
#include "../Renderer/Renderer.h"
#include "../Simulation/SimulationWorld.h"
#include "../Audio/AudioSystem.h"
#include "Input/InputManager.h"
#include "../AssetPipeline/AssetManager.h"
#include "UI/Framework/UISystem.h"
#include "../Renderer/ParticleSystem.h" // Include ParticleSystem header

namespace BGE {

namespace Services {

std::shared_ptr<Renderer> GetRenderer() {
    return ServiceLocator::Instance().GetService<Renderer>();
}

std::shared_ptr<SimulationWorld> GetWorld() {
    return ServiceLocator::Instance().GetService<SimulationWorld>();
}

std::shared_ptr<AudioSystem> GetAudio() {
    return ServiceLocator::Instance().GetService<AudioSystem>();
}

std::shared_ptr<InputManager> GetInput() {
    return ServiceLocator::Instance().GetService<InputManager>();
}

std::shared_ptr<AssetManager> GetAssets() {
    return ServiceLocator::Instance().GetService<AssetManager>();
}

std::shared_ptr<UISystem> GetUI() {
    return ServiceLocator::Instance().GetService<UISystem>();
}

std::shared_ptr<ParticleSystem> GetParticles() {
    return ServiceLocator::Instance().GetService<ParticleSystem>();
}

} // namespace Services

} // namespace BGE