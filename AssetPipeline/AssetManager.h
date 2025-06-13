#pragma once

namespace BGE {

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;
    
    bool Initialize() { return true; }
    void Shutdown() {}
    
    // TODO: Implement asset management functionality
    bool LoadAsset(const char* filename) { (void)filename; return true; }
    void UnloadAsset(const char* filename) { (void)filename; }
};

} // namespace BGE