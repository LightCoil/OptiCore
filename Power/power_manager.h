#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>

class IPowerManager {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetPowerProfile(const std::string& profile) = 0;
    virtual void SetScreenOffTimeout(int seconds) = 0;
    virtual void SetCpuFrequency(int khz) = 0;
    virtual void SetGpuFrequency(int mhz) = 0;
    virtual void EnableDynamicThrottling(bool enable) = 0;
    virtual std::string QueryTelemetry() = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~IPowerManager() {}
}; 