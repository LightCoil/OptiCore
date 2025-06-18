#pragma once
#include "ICommandQueue.h"
#include <queue>
#include <mutex>
#include <condition_variable>

class CommandQueue : public ICommandQueue {
public:
    CommandQueue() = default;
    ~CommandQueue() override = default;

    void push(const GPUCommand& cmd) override;
    bool tryPop(GPUCommand& cmd) override;
    size_t size() const override;
    bool empty() const override;

private:
    mutable std::mutex mtx_;
    std::queue<GPUCommand> queue_;
    std::condition_variable cv_;
}; 