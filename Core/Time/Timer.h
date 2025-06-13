#pragma once

#include <chrono>

namespace BGE {

class Timer {
public:
    Timer();
    
    void Start();
    void Stop();
    void Reset();
    
    float GetElapsedSeconds() const;
    double GetElapsedMilliseconds() const;
    
    bool IsRunning() const { return m_running; }

private:
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_endTime;
    bool m_running;
};

} // namespace BGE