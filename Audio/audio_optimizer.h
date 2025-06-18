#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>
#include <vector>

class IAudioOptimizer {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetBufferSize(int ms) = 0;
    virtual void EnableAsyncMode(bool enable) = 0;
    virtual void SetSampleRate(int hz) = 0;
    virtual void EnableUpsampling(bool enable) = 0;
    virtual void EnableDenoise(bool enable) = 0;
    virtual void SetEQProfile(const std::vector<float>& bands) = 0;
    virtual void AutoEnhance() = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~IAudioOptimizer() {}
}; 