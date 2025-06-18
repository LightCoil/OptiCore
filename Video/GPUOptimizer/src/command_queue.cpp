#include "../include/CommandQueue.h"

void CommandQueue::push(const GPUCommand& cmd) {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.push(cmd);
    cv_.notify_one();
}

bool CommandQueue::tryPop(GPUCommand& cmd) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return false;
    cmd = queue_.front();
    queue_.pop();
    return true;
}

size_t CommandQueue::size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.size();
}

bool CommandQueue::empty() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.empty();
} 