#pragma once
#include "GPUCommand.h"

class ICommandOptimizer {
public:
    virtual ~ICommandOptimizer() = default;
    virtual void optimizeCommand(GPUCommand&) = 0;
    virtual void analyzeCommand(const GPUCommand&) = 0;
    virtual void translateCommand(GPUCommand&) = 0;
}; 