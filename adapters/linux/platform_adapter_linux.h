#pragma once
#include <string>

class IPlatformAdapter {
public:
    virtual bool SetCpuGovernor(const std::string& governor) = 0;
    virtual bool SetSwappiness(int value) = 0;
    virtual bool DropCaches() = 0;
    virtual bool EnableHugePages(bool enable) = 0;
    virtual bool SetIOScheduler(const std::string& device, const std::string& scheduler) = 0;
    virtual bool SetReadAhead(const std::string& device, int kb) = 0;
    virtual bool SetPowerProfile(const std::string& profile) = 0;
    virtual bool SetScreenOffTimeout(int seconds) = 0;
    virtual bool SetGPUPerformanceLevel(const std::string& level) = 0;
    virtual bool SetGPUPowerLimit(int percent) = 0;
    virtual bool SetGPUFanSpeed(int percent) = 0;
    virtual bool SetAudioBufferSize(int ms) = 0;
    virtual bool SetAudioAsyncMode(bool enable) = 0;
    virtual bool SetAudioSampleRate(int hz) = 0;
    virtual ~IPlatformAdapter() {}
}; 