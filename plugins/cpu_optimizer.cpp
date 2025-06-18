#include "cpu_optimizer.h"
#include "../CPU/profiler/performance_monitor.c"
#include "../CPU/profiler/load_predictor.h"
#include "../CPU/microkernel/task_scheduler.c"
#include <sched.h>
#include <fstream>
#include <vector>
#include <string.h>

class CpuOptimizer : public ICpuOptimizer {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
    bool lockfree_enabled = false;
    load_predictor_t predictor{};
public:
    bool Init(IPlatformAdapter* a) override {
        adapter = a;
        load_predictor_init(&predictor, 64);
        status = "initialized";
        return true;
    }
    void Start() override {
        status = "running";
        SetBalancedMode();
    }
    void Stop() override {
        status = "stopped";
    }
    void SetPerformanceMode() override {
        if (adapter) adapter->SetCpuGovernor("performance");
        std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (ofs) ofs << "performance";
        status = "performance";
    }
    void SetPowerSaveMode() override {
        if (adapter) adapter->SetCpuGovernor("powersave");
        std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (ofs) ofs << "powersave";
        status = "powersave";
    }
    void SetBalancedMode() override {
        if (adapter) adapter->SetCpuGovernor("ondemand");
        std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (ofs) ofs << "ondemand";
        status = "balanced";
    }
    void SetAffinity(const std::vector<int>& cpu_ids) override {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        for (int id : cpu_ids) CPU_SET(id, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
        status = "affinity set";
    }
    void SetFrequency(int khz) override {
        std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
        if (ofs) ofs << khz;
        status = "freq=" + std::to_string(khz);
    }
    void SetNUMANode(int node) override {
        // Для простоты: записываем node в status, NUMA через libnuma можно расширить
        status = "numa_node=" + std::to_string(node);
    }
    void EnableLockFreeQueue(bool enable) override {
        lockfree_enabled = enable;
        // task_scheduler поддерживает lock-free очереди
        status = enable ? "lockfree=on" : "lockfree=off";
    }
    void StartProfiler() override {
        init_profiler();
        start_profiling();
        status = "profiler running";
    }
    void StopProfiler() override {
        stop_profiling();
        status = "profiler stopped";
    }
    std::string QueryProfilerReport() override {
        // Можно расширить: возвращать отчёт из буфера
        return "Profiler report: (см. логи)";
    }
    std::string QueryStatus() override {
        return status;
    }
    ~CpuOptimizer() {
        load_predictor_destroy(&predictor);
    }
}; 