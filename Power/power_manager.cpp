#include "power_manager.h"
#include <fstream>
#include <string>

class PowerManager : public IPowerManager {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
    bool dynamic_throttling = false;
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
    void SetPowerProfile(const std::string& profile) override {
        if (adapter) adapter->SetPowerProfile(profile);
        std::ofstream ofs("/sys/class/power_supply/AC/online");
        if (ofs) ofs << (profile == "performance" ? "1" : "0");
        status = "power_profile=" + profile;
    }
    void SetScreenOffTimeout(int seconds) override {
        if (adapter) adapter->SetScreenOffTimeout(seconds);
        std::ofstream ofs("/sys/class/backlight/acpi_video0/bl_power");
        if (ofs) ofs << (seconds == 0 ? "0" : "1");
        status = "screen_timeout=" + std::to_string(seconds);
    }
    void SetCpuFrequency(int khz) override {
        std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
        if (ofs) ofs << khz;
        status = "cpu_freq=" + std::to_string(khz);
    }
    void SetGpuFrequency(int mhz) override {
        std::ofstream ofs("/sys/class/drm/card0/device/pp_dpm_sclk");
        if (ofs) ofs << mhz;
        status = "gpu_freq=" + std::to_string(mhz);
    }
    void EnableDynamicThrottling(bool enable) override {
        dynamic_throttling = enable;
        status = enable ? "dyn_throttle=on" : "dyn_throttle=off";
    }
    std::string QueryTelemetry() override {
        // Пример: чтение температуры и энергопотребления
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        int temp = 0;
        if (temp_file) temp_file >> temp;
        std::ifstream power_file("/sys/class/power_supply/BAT0/power_now");
        int power = 0;
        if (power_file) power_file >> power;
        return "temp=" + std::to_string(temp/1000) + "C, power=" + std::to_string(power/1000000) + "W";
    }
    std::string QueryStatus() override {
        return status;
    }
    ~PowerManager() {}
}; 