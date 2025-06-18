#include "io_optimizer.h"
#include "../Memory/prefetch_manager.h"
#include <fstream>
#include <vector>
#include <string>

class IOOptimizer : public IIOOptimizer {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
    bool async_io = false;
    prefetch_manager_t* prefetch_mgr = nullptr;
public:
    bool Init(IPlatformAdapter* a) override {
        adapter = a;
        status = "initialized";
        return true;
    }
    void Start() override {
        status = "running";
    }
    void Stop() override {
        status = "stopped";
    }
    void SetScheduler(const std::string& device, const std::string& scheduler) override {
        if (adapter) adapter->SetIOScheduler(device, scheduler);
        std::ofstream ofs("/sys/block/" + device + "/queue/scheduler");
        if (ofs) ofs << scheduler;
        status = "scheduler=" + scheduler;
    }
    void SetReadAhead(const std::string& device, int kb) override {
        if (adapter) adapter->SetReadAhead(device, kb);
        std::ofstream ofs("/sys/block/" + device + "/queue/read_ahead_kb");
        if (ofs) ofs << kb;
        status = "readahead=" + std::to_string(kb);
    }
    void EnableAsyncIO(bool enable) override {
        async_io = enable;
        status = enable ? "async_io=on" : "async_io=off";
    }
    void BatchIO(const std::vector<std::string>& ops) override {
        // Пример: batch-обработка IO-операций (можно расширить)
        for (const auto& op : ops) {
            // ... обработка команд ...
        }
        status = "batch_io";
    }
    void IntegratePrefetchManager() override {
        // Интеграция с PrefetchManager (Memory)
        prefetch_mgr = new prefetch_manager_t;
        prefetch_manager_init(prefetch_mgr, PREFETCH_LOOKAHEAD, 4);
        status = "prefetch_integrated";
    }
    std::string QueryTelemetry() override {
        // Пример: чтение IO-метрик
        std::ifstream stat_file("/proc/diskstats");
        std::string line, result;
        while (std::getline(stat_file, line)) result += line + "\n";
        return result;
    }
    std::string QueryStatus() override {
        return status;
    }
    ~IOOptimizer() {
        if (prefetch_mgr) prefetch_manager_destroy(prefetch_mgr);
        delete prefetch_mgr;
    }
}; 