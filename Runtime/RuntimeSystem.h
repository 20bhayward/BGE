#pragma once

namespace BGE {

class RuntimeSystem {
public:
    RuntimeSystem() = default;
    ~RuntimeSystem() = default;
    
    bool Initialize() { return true; }
    void Shutdown() {}
    
    // TODO: Implement runtime functionality
};

} // namespace BGE