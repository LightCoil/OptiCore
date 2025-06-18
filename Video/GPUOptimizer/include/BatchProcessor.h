#pragma once
#include "IBatchProcessor.h"
#include <cstddef>

class BatchProcessor : public IBatchProcessor {
public:
    explicit BatchProcessor(size_t batchSize);
    ~BatchProcessor() override = default;

    void processBatch(const std::vector<GPUCommand>& batch) override;
    size_t getBatchSize() const override;

private:
    size_t batchSize_;
}; 