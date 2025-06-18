#pragma once

class IMemoryExtender {
public:
    virtual ~IMemoryExtender() = default;
    virtual bool extendMemory(size_t) = 0;
    virtual bool swapIn(void* dst, size_t size) = 0;
    virtual bool swapOut(void* src, size_t size) = 0;
    virtual void cacheManagement() = 0;
}; 