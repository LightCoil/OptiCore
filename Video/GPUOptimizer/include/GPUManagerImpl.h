#pragma once
#include "IGPUManager.h"
#include "IPseudoGPU.h"
#include "IMemoryExtender.h"
#include "ICoreEmulator.h"
#include "ICommandOptimizer.h"
#include "IStatisticsManager.h"
#include "IResourceMonitor.h"
#include "CommandQueue.h"
#include "ThreadPool.h"
#include "BatchProcessor.h"
#include <memory>
#include <atomic>
#include <vector>

class GPUManagerImpl : public IGPUManager {
public:
    GPUManagerImpl();
    ~GPUManagerImpl() override;

    void initialize() override;
    void shutdown() override;
    GPUStatus getStatus() const override;
    void routeCommand(const GPUCommand&) override;

    // Новые методы для ускорения
    void submitCommandAsync(const GPUCommand&);
    void processBatches();

private:
    std::unique_ptr<IPseudoGPU> pseudoGPU_;
    std::unique_ptr<IMemoryExtender> memoryExtender_;
    std::unique_ptr<ICoreEmulator> coreEmulator_;
    std::unique_ptr<ICommandOptimizer> commandOptimizer_;
    std::unique_ptr<IStatisticsManager> statisticsManager_;
    std::unique_ptr<IResourceMonitor> resourceMonitor_;
    GPUStatus status_ = GPUStatus::Idle;

    std::unique_ptr<CommandQueue> commandQueue_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<BatchProcessor> batchProcessor_;
    std::atomic<bool> batchingActive_{false};
}; 