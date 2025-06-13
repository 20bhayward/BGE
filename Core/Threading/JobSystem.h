#pragma once

#include <functional>
#include <atomic>
#include <memory>

namespace BGE {

class ThreadPool;

using JobFunction = std::function<void()>;

struct Job {
    JobFunction function;
    std::atomic<bool> completed{false};
    
    Job(JobFunction func) : function(std::move(func)) {}
};

using JobHandle = std::shared_ptr<Job>;

class JobSystem {
public:
    JobSystem();
    ~JobSystem();
    
    bool Initialize(uint32_t numThreads = 0);
    void Shutdown();
    
    JobHandle ScheduleJob(JobFunction function);
    void WaitForJob(JobHandle job);
    void WaitForAllJobs();
    
    uint32_t GetNumThreads() const { return m_numThreads; }
    uint32_t GetQueuedJobCount() const;

private:
    std::unique_ptr<ThreadPool> m_threadPool;
    uint32_t m_numThreads;
    bool m_initialized;
};

} // namespace BGE