#pragma once

#include <string>

namespace BGE {

struct EngineInitializedEvent {
    bool success;
    std::string message;
};

struct EngineShuttingDownEvent {
    std::string reason;
};

struct FrameStartEvent {
    float deltaTime;
    uint64_t frameCount;
};

struct FrameEndEvent {
    float deltaTime;
    uint64_t frameCount;
    float frameTime;
};

struct WindowResizeEvent {
    uint32_t width;
    uint32_t height;
};

struct ApplicationStateChangedEvent {
    enum State {
        Initializing,
        Running,
        Paused,
        Shutting_Down
    } state;
};

} // namespace BGE