#pragma once
#include "IThreadPool.h"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool : public IThreadPool {
public:
    explicit ThreadPool(size_t threadCount);
    ~ThreadPool() override;

    void enqueue(const std::function<void()>& task) override;
    size_t getThreadCount() const override;
    void waitAll() override;

private:
    void worker();
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
}; 