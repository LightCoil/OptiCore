#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>
#include <vector>

class IGPUOptimizer {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetPerformanceLevel(const std::string& level) = 0;
    virtual void SetPowerLimit(int percent) = 0;
    virtual void SetFanSpeed(int percent) = 0;
    virtual void BatchOptimize(const std::vector<std::string>& commands) = 0;
    virtual void CompressTexture(const std::vector<float>& input, std::vector<float>& output, int levels) = 0;
    virtual void EnableAsync(bool enable) = 0;
    virtual std::string QueryTelemetry() = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~IGPUOptimizer() {}
}; 