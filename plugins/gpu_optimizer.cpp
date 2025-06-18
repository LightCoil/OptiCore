#include "gpu_optimizer.h"
#include "../Video/GPUOptimizer/include/wavelet_compressor.h"
#include <fstream>
#include <vector>
#include <string>

class GPUOptimizer : public IGPUOptimizer {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
    bool async_enabled = false;
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
    void SetPerformanceLevel(const std::string& level) override {
        if (adapter) adapter->SetGPUPerformanceLevel(level);
        std::ofstream ofs("/sys/class/drm/card0/device/power_dpm_force_performance_level");
        if (ofs) ofs << level;
        status = "gpu_level=" + level;
    }
    void SetPowerLimit(int percent) override {
        if (adapter) adapter->SetGPUPowerLimit(percent);
        std::ofstream ofs("/sys/class/drm/card0/device/power1_cap");
        if (ofs) ofs << percent;
        status = "gpu_power=" + std::to_string(percent);
    }
    void SetFanSpeed(int percent) override {
        if (adapter) adapter->SetGPUFanSpeed(percent);
        std::ofstream ofs("/sys/class/hwmon/hwmon0/pwm1");
        if (ofs) ofs << percent * 2.55; // 0-255
        status = "gpu_fan=" + std::to_string(percent);
    }
    void BatchOptimize(const std::vector<std::string>& commands) override {
        // Пример: batch-обработка команд (можно расширить)
        for (const auto& cmd : commands) {
            // ... обработка команд ...
        }
        status = "batch optimized";
    }
    void CompressTexture(const std::vector<float>& input, std::vector<float>& output, int levels) override {
        WaveletCompressor::compress(input, output, levels);
        status = "texture compressed";
    }
    void EnableAsync(bool enable) override {
        async_enabled = enable;
        status = enable ? "async=on" : "async=off";
    }
    std::string QueryTelemetry() override {
        // Пример: чтение температуры и частоты
        std::ifstream temp_file("/sys/class/drm/card0/device/hwmon/hwmon0/temp1_input");
        int temp = 0;
        if (temp_file) temp_file >> temp;
        std::ifstream freq_file("/sys/class/drm/card0/device/pp_dpm_sclk");
        std::string freq;
        if (freq_file) std::getline(freq_file, freq);
        return "temp=" + std::to_string(temp/1000) + "C, freq=" + freq;
    }
    std::string QueryStatus() override {
        return status;
    }
    ~GPUOptimizer() {}
}; 