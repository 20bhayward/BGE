#pragma once

namespace BGE {

class EditorSystem {
public:
    EditorSystem() = default;
    ~EditorSystem() = default;
    
    bool Initialize() { return true; }
    void Shutdown() {}
    void Update(float deltaTime) {}
    void Render() {}
    
    // TODO: Implement editor functionality
};

} // namespace BGE