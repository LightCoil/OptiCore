#pragma once

struct MonitorMetrics {
    float cpuLoad;
    float cpuTemp;
    float memUsage;
    float ioThroughput;
    // ... другие метрики
};

class IMonitor {
public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual MonitorMetrics GetCurrentMetrics() = 0;
    virtual ~IMonitor() {}
}; 