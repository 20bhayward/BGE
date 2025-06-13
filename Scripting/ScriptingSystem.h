#pragma once

namespace BGE {

class ScriptingSystem {
public:
    ScriptingSystem() = default;
    ~ScriptingSystem() = default;
    
    bool Initialize() { return true; }
    void Shutdown() {}
    void Update(float deltaTime) {}
    
    // TODO: Implement scripting functionality
    bool LoadScript(const char* filename) { return true; }
    void ExecuteScript(const char* script) {}
};

} // namespace BGE