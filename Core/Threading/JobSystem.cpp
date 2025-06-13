#include "JobSystem.h"
#include "ThreadPool.h"

namespace BGE {

JobSystem::JobSystem() : m_numThreads(0), m_initialized(false) {
}

JobSystem::~JobSystem() {
    Shutdown();
}

bool JobSystem::Initialize(uint32_t numThreads) {
    if (m_initialized) return true;
    
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
    }
    
    m_numThreads = numThreads;
    m_threadPool = std::make_unique<ThreadPool>(numThreads);
    m_initialized = true;
    
    return true;
}

void JobSystem::Shutdown() {
    if (m_initialized) {
        WaitForAllJobs();
        m_threadPool.reset();
        m_initialized = false;
    }
}

JobHandle JobSystem::ScheduleJob(JobFunction function) {
    if (!m_initialized) return nullptr;
    
    auto job = std::make_shared<Job>(std::move(function));
    
    m_threadPool->Enqueue([job]() {
        job->function();
        job->completed.store(true);
    });
    
    return job;
}

void JobSystem::WaitForJob(JobHandle job) {
    if (!job) return;
    
    while (!job->completed.load()) {
        std::this_thread::yield();
    }
}

void JobSystem::WaitForAllJobs() {
    // TODO: Implement proper job completion tracking
    // For now, just yield a bit to let jobs complete
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

uint32_t JobSystem::GetQueuedJobCount() const {
    // TODO: Implement proper job queue size tracking
    return 0;
}

} // namespace BGE