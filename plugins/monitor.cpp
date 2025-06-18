#include "monitor.h"
#include <chrono>
#include <thread>

class Monitor : public IMonitor {
    MonitorMetrics metrics;
    bool running = false;
    std::thread monitorThread;

    void MonitorLoop() {
        while (running) {
            // TODO: Реальные вызовы к /proc/stat, /proc/meminfo, sensors и т.д.
            metrics.cpuLoad = 0.0f; // заглушка
            metrics.cpuTemp = 0.0f; // заглушка
            metrics.memUsage = 0.0f; // заглушка
            metrics.ioThroughput = 0.0f; // заглушка
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
public:
    void Start() override {
        running = true;
        monitorThread = std::thread(&Monitor::MonitorLoop, this);
    }
    void Stop() override {
        running = false;
        if (monitorThread.joinable()) monitorThread.join();
    }
    MonitorMetrics GetCurrentMetrics() override {
        return metrics;
    }
    ~Monitor() {
        Stop();
    }
}; 