#include "Timer.h"

namespace BGE {

Timer::Timer() : m_running(false) {
    Reset();
}

void Timer::Start() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_running = true;
}

void Timer::Stop() {
    m_endTime = std::chrono::high_resolution_clock::now();
    m_running = false;
}

void Timer::Reset() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_endTime = m_startTime;
    m_running = false;
}

float Timer::GetElapsedSeconds() const {
    auto endTime = m_running ? std::chrono::high_resolution_clock::now() : m_endTime;
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - m_startTime);
    return static_cast<float>(duration.count()) * 1e-9f;
}

double Timer::GetElapsedMilliseconds() const {
    auto endTime = m_running ? std::chrono::high_resolution_clock::now() : m_endTime;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
    return static_cast<double>(duration.count()) * 1e-3;
}

} // namespace BGE