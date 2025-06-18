#include "../include/ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount) {
    for (size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this]() { this->worker(); });
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void ThreadPool::enqueue(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        tasks_.push(task);
    }
    cv_.notify_one();
}

size_t ThreadPool::getThreadCount() const {
    return workers_.size();
}

void ThreadPool::waitAll() {
    while (true) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (tasks_.empty()) break;
    }
}

void ThreadPool::worker() {
    while (!stop_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) return;
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        if (task) task();
    }
} 