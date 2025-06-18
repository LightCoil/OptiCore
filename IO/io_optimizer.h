#pragma once
#include "../adapters/linux/platform_adapter_linux.h"
#include <string>
#include <vector>

class IIOOptimizer {
public:
    virtual bool Init(IPlatformAdapter* adapter) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void SetScheduler(const std::string& device, const std::string& scheduler) = 0;
    virtual void SetReadAhead(const std::string& device, int kb) = 0;
    virtual void EnableAsyncIO(bool enable) = 0;
    virtual void BatchIO(const std::vector<std::string>& ops) = 0;
    virtual void IntegratePrefetchManager() = 0;
    virtual std::string QueryTelemetry() = 0;
    virtual std::string QueryStatus() = 0;
    virtual ~IIOOptimizer() {}
}; 