#include "../include/BatchProcessor.h"
#include <iostream>

BatchProcessor::BatchProcessor(size_t batchSize) : batchSize_(batchSize) {}

void BatchProcessor::processBatch(const std::vector<GPUCommand>& batch) {
    std::cout << "BatchProcessor: обработка батча из " << batch.size() << " команд" << std::endl;
    // Здесь может быть оптимизация, SIMD, параллелизм и т.д.
}

size_t BatchProcessor::getBatchSize() const {
    return batchSize_;
} 