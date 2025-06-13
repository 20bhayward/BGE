#pragma once

#include <chrono>

namespace BGE {

class Clock {
public:
    Clock();
    
    void Update();
    
    float GetDeltaTime() const { return m_deltaTime; }
    float GetTotalTime() const { return m_totalTime; }
    uint64_t GetFrameCount() const { return m_frameCount; }
    float GetFPS() const { return m_fps; }

private:
    std::chrono::high_resolution_clock::time_point m_lastTime;
    std::chrono::high_resolution_clock::time_point m_startTime;
    
    float m_deltaTime;
    float m_totalTime;
    uint64_t m_frameCount;
    float m_fps;
    
    // FPS calculation
    float m_fpsTimer;
    uint32_t m_fpsFrameCount;
    
    static constexpr float FPS_UPDATE_INTERVAL = 1.0f; // Update FPS every second
};

} // namespace BGE