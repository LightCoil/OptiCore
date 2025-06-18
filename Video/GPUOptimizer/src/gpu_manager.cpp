#include "../include/IGPUManager.h"
#include "../include/GPUManagerImpl.h"
#include "../include/IPseudoGPU.h"
#include "../include/IMemoryExtender.h"
#include "../include/ICoreEmulator.h"
#include "../include/ICommandOptimizer.h"
#include "../include/IStatisticsManager.h"
#include "../include/IResourceMonitor.h"

// Реализации модулей
#include "../src/pseudo_gpu.cpp"
#include "../src/memory_extender.cpp"
#include "../src/core_emulator.cpp"
#include "../src/command_optimizer.cpp"
#include "../src/statistics_manager.cpp"
#include "../src/system_utils.cpp"

#include <thread>

class GPUManager : public IGPUManager {
public:
    GPUManager() = default;
    ~GPUManager() override = default;

    void initialize() override {
        pseudoGPU_ = std::make_unique<PseudoGPU>();
        memoryExtender_ = std::make_unique<MemoryExtender>();
        coreEmulator_ = std::make_unique<CoreEmulator>();
        commandOptimizer_ = std::make_unique<CommandOptimizer>();
        statisticsManager_ = std::make_unique<StatisticsManager>();
        resourceMonitor_ = std::make_unique<ResourceMonitor>();
        commandQueue_ = std::make_unique<CommandQueue>();
        threadPool_ = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
        batchProcessor_ = std::make_unique<BatchProcessor>(32); // размер батча по умолчанию
        batchingActive_ = true;
        // Запуск обработки батчей в отдельном потоке
        threadPool_->enqueue([this]() { this->processBatches(); });
        status_ = GPUStatus::Idle;
    }

    void shutdown() override {
        batchingActive_ = false;
        threadPool_->waitAll();
        resourceMonitor_.reset();
        statisticsManager_.reset();
        commandOptimizer_.reset();
        coreEmulator_.reset();
        memoryExtender_.reset();
        pseudoGPU_.reset();
        batchProcessor_.reset();
        commandQueue_.reset();
        threadPool_.reset();
        status_ = GPUStatus::Idle;
    }

    GPUStatus getStatus() const override {
        return status_;
    }

    void routeCommand(const GPUCommand& cmd) override {
        submitCommandAsync(cmd);
    }

    void submitCommandAsync(const GPUCommand& cmd) {
        if (commandQueue_) commandQueue_->push(cmd);
    }

    void processBatches() {
        std::vector<GPUCommand> batch;
        while (batchingActive_) {
            batch.clear();
            // Собираем батч
            while (batch.size() < batchProcessor_->getBatchSize()) {
                GPUCommand cmd;
                if (commandQueue_ && commandQueue_->tryPop(cmd)) {
                    batch.push_back(cmd);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            if (!batch.empty()) {
                // Обработка батча в пуле потоков
                threadPool_->enqueue([this, batch]() {
                    batchProcessor_->processBatch(batch);
                    // Здесь можно добавить вызовы оптимизации, статистики и мониторинга
                });
            }
        }
    }

private:
    std::unique_ptr<PseudoGPU> pseudoGPU_;
    std::unique_ptr<MemoryExtender> memoryExtender_;
    std::unique_ptr<CoreEmulator> coreEmulator_;
    std::unique_ptr<CommandOptimizer> commandOptimizer_;
    std::unique_ptr<StatisticsManager> statisticsManager_;
    std::unique_ptr<ResourceMonitor> resourceMonitor_;
    std::unique_ptr<CommandQueue> commandQueue_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<BatchProcessor> batchProcessor_;
    bool batchingActive_;
    GPUStatus status_;
}; 