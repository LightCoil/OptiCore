#pragma once
#include "GPUCommand.h"
#include <vector>

class IBatchProcessor {
public:
    virtual ~IBatchProcessor() = default;
    virtual void processBatch(const std::vector<GPUCommand>& batch) = 0;
    virtual size_t getBatchSize() const = 0;
}; 