#pragma once
#include <functional>
#include <cstddef>

class IThreadPool {
public:
    virtual ~IThreadPool() = default;
    virtual void enqueue(const std::function<void()>& task) = 0;
    virtual size_t getThreadCount() const = 0;
    virtual void waitAll() = 0;
}; 