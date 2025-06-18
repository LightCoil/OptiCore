#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>

class IAudioOptimizer {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetBufferSize(int ms) = 0;
    virtual void EnableAsyncMode(bool enable) = 0;
    virtual void SetSampleRate(int hz) = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~IAudioOptimizer() {}
}; 