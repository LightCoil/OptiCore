#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>
#include <vector>

class IMemoryOptimizer {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetSwappiness(int value) = 0;
    virtual void DropCaches() = 0;
    virtual void EnableHugePages(bool enable) = 0;
    virtual bool SwapLoadBlock(uint64_t block_id, void** out_data) = 0;
    virtual bool SwapWritebackBlock(uint64_t block_id) = 0;
    virtual void SwapEvictLRU() = 0;
    virtual void PrefetchBlocks(const std::vector<uint64_t>& block_ids) = 0;
    virtual size_t QueryHugePagesUsage() = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~IMemoryOptimizer() {}
}; 