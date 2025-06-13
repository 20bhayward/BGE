#include "Clock.h"

namespace BGE {

Clock::Clock() 
    : m_deltaTime(0.0f)
    , m_totalTime(0.0f)
    , m_frameCount(0)
    , m_fps(0.0f)
    , m_fpsTimer(0.0f)
    , m_fpsFrameCount(0) {
    
    m_lastTime = std::chrono::high_resolution_clock::now();
    m_startTime = m_lastTime;
}

void Clock::Update() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    
    // Calculate delta time
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - m_lastTime);
    m_deltaTime = static_cast<float>(duration.count()) * 1e-9f;
    
    // Calculate total time
    auto totalDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - m_startTime);
    m_totalTime = static_cast<float>(totalDuration.count()) * 1e-9f;
    
    // Update frame count
    ++m_frameCount;
    
    // Update FPS calculation
    m_fpsTimer += m_deltaTime;
    ++m_fpsFrameCount;
    
    if (m_fpsTimer >= FPS_UPDATE_INTERVAL) {
        m_fps = static_cast<float>(m_fpsFrameCount) / m_fpsTimer;
        m_fpsTimer = 0.0f;
        m_fpsFrameCount = 0;
    }
    
    m_lastTime = currentTime;
}

} // namespace BGE