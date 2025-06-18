#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>
#include <vector>

class ICpuOptimizer {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetPerformanceMode() = 0;
    virtual void SetPowerSaveMode() = 0;
    virtual void SetBalancedMode() = 0;
    virtual void SetAffinity(const std::vector<int>& cpu_ids) = 0;
    virtual void SetFrequency(int khz) = 0;
    virtual void SetNUMANode(int node) = 0;
    virtual void EnableLockFreeQueue(bool enable) = 0;
    virtual void StartProfiler() = 0;
    virtual void StopProfiler() = 0;
    virtual std::string QueryProfilerReport() = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~ICpuOptimizer() {}
}; 