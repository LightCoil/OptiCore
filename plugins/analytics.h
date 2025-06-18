#pragma once
#include <string>

enum class AnalysisType {
    FullSystem,
    CPU,
    IO,
    Memory,
    Custom
};

struct AnalysisReport {
    AnalysisType type;
    float performanceGainPercent;
    float tempReductionCelsius;
    float energySavingPercent;
    std::string summary;
    // ... другие поля
};

class IAnalytics {
public:
    virtual void StartAnalysis(AnalysisType type, int duration_sec) = 0;
    virtual AnalysisReport GetLastReport() = 0;
    virtual bool IsAnalysisRunning() = 0;
    virtual ~IAnalytics() {}
}; 