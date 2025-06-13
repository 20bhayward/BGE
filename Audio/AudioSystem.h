#pragma once

namespace BGE {

class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem() = default;
    
    bool Initialize() { return true; }
    void Shutdown() {}
    void Update(float deltaTime) { (void)deltaTime; }
    
    // TODO: Implement audio functionality
    void PlaySound(const char* filename) { (void)filename; }
    void SetVolume(float volume) { (void)volume; }
};

} // namespace BGE