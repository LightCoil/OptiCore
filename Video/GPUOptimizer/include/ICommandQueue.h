#pragma once
#include "GPUCommand.h"
#include <memory>

class ICommandQueue {
public:
    virtual ~ICommandQueue() = default;
    virtual void push(const GPUCommand&) = 0;
    virtual bool tryPop(GPUCommand&) = 0;
    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
}; 