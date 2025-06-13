#include "ThreadPool.h"
#include <algorithm>

namespace BGE {

thread_local size_t ThreadPool::t_threadId = 0;

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // Fallback
    }
    
    InitializeThreads();
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::InitializeThreads() {
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    
    for (size_t i = 0; i < numThreads; ++i) {
        m_threads.emplace_back(&ThreadPool::WorkerThread, this, i);
    }
}

void ThreadPool::WorkerThread(size_t threadId) {
    t_threadId = threadId;
    
    while (!m_shutdown.load()) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { 
                return m_shutdown.load() || !m_tasks.empty(); 
            });
            
            if (m_shutdown.load() && m_tasks.empty()) {
                return;
            }
            
            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
                ++m_activeTasks;
            }
        }
        
        if (task) {
            task();
            --m_activeTasks;
            ++m_completedTasks;
            m_finished.notify_all();
        }
    }
}

void ThreadPool::WaitForAll() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_finished.wait(lock, [this] {
        return m_tasks.empty() && m_activeTasks.load() == 0;
    });
}

void ThreadPool::Resize(size_t numThreads) {
    Shutdown();
    m_shutdown.store(false);
    
    for (size_t i = 0; i < numThreads; ++i) {
        m_threads.emplace_back(&ThreadPool::WorkerThread, this, i);
    }
}

void ThreadPool::Shutdown() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_shutdown.store(true);
    }
    
    m_condition.notify_all();
    
    for (std::thread& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    m_threads.clear();
}

void ThreadPool::StopAllThreads() {
    Shutdown();
}

size_t ThreadPool::GetQueueSize() const {
    std::unique_lock<std::mutex> lock(const_cast<std::mutex&>(m_queueMutex));
    return m_tasks.size();
}


} // namespace BGE