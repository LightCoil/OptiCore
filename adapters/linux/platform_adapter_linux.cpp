#include "platform_adapter_linux.h"
#include <fstream>
#include <string>
#include <cstdlib>
#include <iostream>
#include <syslog.h>
#include <unistd.h>

class PlatformAdapterLinux : public IPlatformAdapter {
public:
    bool SetCpuGovernor(const std::string& governor) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set CPU governor");
            return false;
        }
        std::string path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
        std::ofstream file(path);
        if (!file.is_open()) {
        syslog(LOG_ERR, "Failed to open %s for writing", path.c_str());
            return false;
        }
        file << governor;
        if (file.fail()) {
        syslog(LOG_ERR, "Failed to write governor value to %s", path.c_str());
            return false;
        }
        syslog(LOG_INFO, "CPU governor set to %s", governor.c_str());
        return true;
    }
    
    bool SetSwappiness(int value) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set swappiness");
            return false;
        }
        if (value < 0 || value > 100) {
        syslog(LOG_ERR, "Swappiness must be between 0 and 100");
            return false;
        }
        std::string path = "/proc/sys/vm/swappiness";
        std::ofstream file(path);
        if (!file.is_open()) {
        syslog(LOG_ERR, "Failed to open %s for writing", path.c_str());
            return false;
        }
        file << value;
        if (file.fail()) {
        syslog(LOG_ERR, "Failed to write swappiness value to %s", path.c_str());
            return false;
        }
        syslog(LOG_INFO, "Swappiness set to %d", value);
        return true;
    }
    
    bool DropCaches() override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to drop caches");
            return false;
        }
        std::string path = "/proc/sys/vm/drop_caches";
        std::ofstream file(path);
        if (!file.is_open()) {
        syslog(LOG_ERR, "Failed to open %s for writing", path.c_str());
            return false;
        }
        file << 3; // Очистка всех кэшей (pagecache, dentries, inodes)
        if (file.fail()) {
        syslog(LOG_ERR, "Failed to write drop caches value to %s", path.c_str());
            return false;
        }
        syslog(LOG_INFO, "Memory caches dropped");
        return true;
    }
    
    bool EnableHugePages(bool enable) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to manage hugepages");
            return false;
        }
        std::string path = "/proc/sys/vm/nr_hugepages";
        std::ofstream file(path);
        if (!file.is_open()) {
        syslog(LOG_ERR, "Failed to open %s for writing", path.c_str());
            return false;
        }
        int hugepages_count = enable ? 128 : 0; // Устанавливаем 128 hugepages при включении, 0 при выключении
        file << hugepages_count;
        if (file.fail()) {
        syslog(LOG_ERR, "Failed to write hugepages value to %s", path.c_str());
            return false;
        }
        syslog(LOG_INFO, "Hugepages %s with count %d", enable ? "enabled" : "disabled", hugepages_count);
        return true;
    }
    
    bool SetIOScheduler(const std::string& device, const std::string& scheduler) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set IO scheduler");
            return false;
        }
        std::string path = "/sys/block/" + device + "/queue/scheduler";
        std::ofstream file(path);
        if (!file.is_open()) {
        syslog(LOG_ERR, "Failed to open %s for writing", path.c_str());
            return false;
        }
        file << scheduler;
        if (file.fail()) {
        syslog(LOG_ERR, "Failed to write scheduler value to %s", path.c_str());
            return false;
        }
        syslog(LOG_INFO, "IO scheduler for device %s set to %s", device.c_str(), scheduler.c_str());
        return true;
    }
    
    bool SetReadAhead(const std::string& device, int kb) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set read_ahead_kb");
            return false;
        }
        if (kb < 0) {
        syslog(LOG_ERR, "read_ahead_kb cannot be negative");
            return false;
        }
        std::string path = "/sys/block/" + device + "/queue/read_ahead_kb";
        std::ofstream file(path);
        if (!file.is_open()) {
        syslog(LOG_ERR, "Failed to open %s for writing", path.c_str());
            return false;
        }
        file << kb;
        if (file.fail()) {
        syslog(LOG_ERR, "Failed to write read_ahead_kb value to %s", path.c_str());
            return false;
        }
        syslog(LOG_INFO, "Read ahead for device %s set to %d KB", device.c_str(), kb);
        return true;
    }
    
    bool SetPowerProfile(const std::string& profile) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set power profile");
            return false;
        }
        std::string command = "cpufreq-set -g ";
        if (profile == "performance") {
            command += "performance";
        } else if (profile == "powersave") {
            command += "powersave";
        } else if (profile == "ondemand") {
            command += "ondemand";
        } else {
        syslog(LOG_ERR, "Invalid power profile. Valid values: performance, powersave, ondemand");
            return false;
        }
        if (system(command.c_str()) != 0) {
        syslog(LOG_ERR, "Failed to set power profile using cpufreq-set");
            return false;
        }
        syslog(LOG_INFO, "Power profile set to %s", profile.c_str());
        return true;
    }
    
    bool SetScreenOffTimeout(int seconds) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set screen off timeout");
            return false;
        }
        if (seconds < 0) {
        syslog(LOG_ERR, "Timeout cannot be negative");
            return false;
        }
        std::string command = "xset dpms " + std::to_string(seconds) + " " + std::to_string(seconds) + " " + std::to_string(seconds);
        if (system(command.c_str()) != 0) {
        syslog(LOG_ERR, "Failed to set screen off timeout using xset");
            return false;
        }
        syslog(LOG_INFO, "Screen off timeout set to %d seconds", seconds);
        return true;
    }
    
    bool SetGPUPerformanceLevel(const std::string& level) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set GPU performance level");
            return false;
        }
        std::string command;
        if (level == "low" || level == "high" || level == "auto") {
            // Для NVIDIA используем nvidia-smi
            command = "nvidia-smi -pl ";
            if (level == "low") {
                command += "50"; // 50W для низкой производительности
            } else if (level == "high") {
                command += "300"; // 300W для высокой производительности
            } else {
                command += "150"; // 150W для автоматического режима
            }
            if (system(command.c_str()) == 0) {
        syslog(LOG_INFO, "GPU performance level set to %s (NVIDIA)", level.c_str());
                return true;
            }
            // Для AMD используем radeon-profile
            command = "radeon-profile --set-power-level " + level;
            if (system(command.c_str()) == 0) {
        syslog(LOG_INFO, "GPU performance level set to %s (AMD)", level.c_str());
                return true;
            }
        syslog(LOG_ERR, "Failed to set GPU performance level");
            return false;
        } else {
        syslog(LOG_ERR, "Invalid GPU performance level. Valid values: low, auto, high");
            return false;
        }
    }
    
    bool SetGPUPowerLimit(int percent) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set GPU power limit");
            return false;
        }
        if (percent < 10 || percent > 120) {
        syslog(LOG_ERR, "GPU power limit must be between 10%% and 120%%");
            return false;
        }
        // Для NVIDIA используем nvidia-smi
        int powerLimitWatts = (percent * 300) / 100; // Предполагаем, что 100% = 300W
        std::string command = "nvidia-smi -pl " + std::to_string(powerLimitWatts);
        if (system(command.c_str()) == 0) {
        syslog(LOG_INFO, "GPU power limit set to %d%% (%dW) (NVIDIA)", percent, powerLimitWatts);
            return true;
        }
        // Для AMD используем radeon-profile
        std::string level;
        if (percent <= 50) {
            level = "low";
        } else if (percent >= 90) {
            level = "high";
        } else {
            level = "auto";
        }
        command = "radeon-profile --set-power-level " + level;
        if (system(command.c_str()) == 0) {
        syslog(LOG_INFO, "GPU power limit set to %d%% (AMD)", percent);
            return true;
        }
        syslog(LOG_ERR, "Failed to set GPU power limit");
        return false;
    }
    
    bool SetGPUFanSpeed(int percent) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set GPU fan speed");
            return false;
        }
        if (percent < 0 || percent > 100) {
        syslog(LOG_ERR, "GPU fan speed must be between 0%% and 100%%");
            return false;
        }
        // Для NVIDIA используем nvidia-settings
        std::string manualCommand = "nvidia-settings -a [gpu:0]/GPUFanControlState=1";
        std::string fanCommand = "nvidia-settings -a [fan:0]/GPUTargetFanSpeed=" + std::to_string(percent);
        if (system(manualCommand.c_str()) == 0 && system(fanCommand.c_str()) == 0) {
        syslog(LOG_INFO, "GPU fan speed set to %d%% (NVIDIA)", percent);
            return true;
        }
        // Для AMD используем radeon-profile
        std::string command = "radeon-profile --set-fan-speed " + std::to_string(percent);
        if (system(command.c_str()) == 0) {
        syslog(LOG_INFO, "GPU fan speed set to %d%% (AMD)", percent);
            return true;
        }
        syslog(LOG_ERR, "Failed to set GPU fan speed");
        return false;
    }
    
    bool SetAudioBufferSize(int ms) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set audio buffer size");
            return false;
        }
        if (ms < 1 || ms > 1000) {
        syslog(LOG_ERR, "Audio buffer size must be between 1 and 1000 ms");
            return false;
        }
        // Настройка через PulseAudio
        std::string command = "pactl load-module module-ladspa-sink buffer_time=" + std::to_string(ms * 1000);
        if (system(command.c_str()) != 0) {
        syslog(LOG_ERR, "Failed to set audio buffer size using PulseAudio");
            return false;
        }
        syslog(LOG_INFO, "Audio buffer size set to %d ms", ms);
        return true;
    }
    
    bool SetAudioAsyncMode(bool enable) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set audio async mode");
            return false;
        }
        // Настройка через PulseAudio
        std::string command = enable ? "pactl load-module module-ladspa-sink async=1" : "pactl unload-module module-ladspa-sink";
        if (system(command.c_str()) != 0) {
        syslog(LOG_ERR, "Failed to %s audio async mode using PulseAudio", enable ? "enable" : "disable");
            return false;
        }
        syslog(LOG_INFO, "Audio async mode %s", enable ? "enabled" : "disabled");
        return true;
    }
    
    bool SetAudioSampleRate(int hz) override {
        if (geteuid() != 0) {
        syslog(LOG_ERR, "Root privileges required to set audio sample rate");
            return false;
        }
        if (hz < 8000 || hz > 192000) {
        syslog(LOG_ERR, "Audio sample rate must be between 8000 and 192000 Hz");
            return false;
        }
        // Настройка через PulseAudio
        std::string command = "pactl load-module module-ladspa-sink rate=" + std::to_string(hz);
        if (system(command.c_str()) != 0) {
        syslog(LOG_ERR, "Failed to set audio sample rate using PulseAudio");
            return false;
        }
        syslog(LOG_INFO, "Audio sample rate set to %d Hz", hz);
        return true;
    }
};
