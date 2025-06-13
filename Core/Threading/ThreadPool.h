#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace BGE {

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = 0); // 0 = auto-detect
    ~ThreadPool();
    
    // Task submission
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>>;
    
    // Simple enqueue method for compatibility
    template<typename F>
    auto Enqueue(F&& f) -> std::future<typename std::invoke_result_t<F>> {
        return Submit(std::forward<F>(f));
    }
    
    // Bulk task submission
    template<typename Iterator, typename Function>
    void SubmitRange(Iterator begin, Iterator end, Function func);
    
    // Parallel for loop
    template<typename Function>
    void ParallelFor(size_t start, size_t end, Function func, size_t grainSize = 1);
    
    // Wait for all tasks to complete
    void WaitForAll();
    
    // Pool management
    void Resize(size_t numThreads);
    void Shutdown();
    bool IsShutdown() const { return m_shutdown.load(); }
    
    // Statistics
    size_t GetThreadCount() const { return m_threads.size(); }
    size_t GetQueueSize() const;
    size_t GetCompletedTasks() const { return m_completedTasks.load(); }
    
    // Thread affinity (optional optimization)
    void SetThreadAffinity(bool enable) { m_useAffinity = enable; }
    
private:
    void WorkerThread(size_t threadId);
    void InitializeThreads();
    void StopAllThreads();
    
    // Thread management
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_useAffinity{false};
    
    // Task queue
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::condition_variable m_finished;
    
    // Statistics
    std::atomic<size_t> m_activeTasks{0};
    std::atomic<size_t> m_completedTasks{0};
    
    // Thread-local random state for work stealing
    thread_local static size_t t_threadId;
};

// Template implementations
template<typename F, typename... Args>
auto ThreadPool::Submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>> {
    using ReturnType = typename std::invoke_result_t<F, Args...>;
    
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<ReturnType> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        if (m_shutdown.load()) {
            throw std::runtime_error("Cannot submit task to shutdown thread pool");
        }
        
        m_tasks.emplace([task](){ (*task)(); });
        ++m_activeTasks;
    }
    
    m_condition.notify_one();
    return result;
}

template<typename Iterator, typename Function>
void ThreadPool::SubmitRange(Iterator begin, Iterator end, Function func) {
    std::vector<std::future<void>> futures;
    
    for (auto it = begin; it != end; ++it) {
        futures.emplace_back(Submit([func, it]() { func(*it); }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
}

template<typename Function>
void ThreadPool::ParallelFor(size_t start, size_t end, Function func, size_t grainSize) {
    if (start >= end) return;
    
    size_t numThreads = GetThreadCount();
    size_t totalWork = end - start;
    size_t workPerThread = std::max(grainSize, totalWork / numThreads);
    
    std::vector<std::future<void>> futures;
    
    for (size_t i = start; i < end; i += workPerThread) {
        size_t rangeEnd = std::min(i + workPerThread, end);
        
        futures.emplace_back(Submit([func, i, rangeEnd]() {
            for (size_t j = i; j < rangeEnd; ++j) {
                func(j);
            }
        }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
}

// Work-stealing thread pool for better load balancing
class WorkStealingThreadPool {
public:
    explicit WorkStealingThreadPool(size_t numThreads = 0);
    ~WorkStealingThreadPool();
    
    template<typename F>
    std::future<typename std::invoke_result_t<F>> Submit(F f);
    
    void Shutdown();

private:
    class WorkStealingQueue {
    public:
        void Push(std::function<void()> task);
        bool TryPop(std::function<void()>& task);
        bool TrySteal(std::function<void()>& task);
        
    private:
        std::deque<std::function<void()>> m_queue;
        mutable std::mutex m_mutex;
    };
    
    void WorkerThread(size_t threadId);
    bool TryStealWork(std::function<void()>& task, size_t thiefId);
    
    std::vector<std::thread> m_threads;
    std::vector<std::unique_ptr<WorkStealingQueue>> m_queues;
    std::atomic<bool> m_shutdown{false};
    
    thread_local static size_t t_threadIndex;
    thread_local static WorkStealingQueue* t_localQueue;
};

} // namespace BGE