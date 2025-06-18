#pragma once
#include "GPUCommand.h"

class IPseudoGPU {
public:
    virtual ~IPseudoGPU() = default;
    virtual void processCommand(const GPUCommand&) = 0;
    virtual void* allocateVirtualMemory(size_t) = 0;
    virtual void releaseVirtualMemory(void*) = 0;
}; 