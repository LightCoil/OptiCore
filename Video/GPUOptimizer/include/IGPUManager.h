#pragma once
#include "GPUCommand.h"
#include "GPUStatus.h"

class IGPUManager {
public:
    virtual ~IGPUManager() = default;
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual GPUStatus getStatus() const = 0;
    virtual void routeCommand(const GPUCommand&) = 0;
}; 