#include "gpu_optimizer.h"
#include <iostream>
#include <string>
#include <syslog.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unistd.h>

enum class GPUVendor {
    NVIDIA,
    AMD,
    INTEL,
    UNKNOWN
};

class GPUOptimizer : public IGPUOptimizer {
private:
    bool isInitialized;
    std::string currentPerformanceLevel;
    int currentPowerLimit;
    int currentFanSpeed;
    bool autoOptimizationEnabled;
    float currentTemperature;
    float currentLoad;
    GPUVendor detectedVendor;
    int gpuIndex;
    bool isRoot;

    bool checkRoot() {
        return geteuid() == 0;
    }

    GPUVendor detectGPUVendor() {
        std::string command = "lspci | grep -i 'vga\\|3d\\|2d' | grep -i 'nvidia'";
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                pclose(pipe);
                return GPUVendor::NVIDIA;
            }
            pclose(pipe);
        }

        command = "lspci | grep -i 'vga\\|3d\\|2d' | grep -i 'amd\\|ati'";
        pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                pclose(pipe);
                return GPUVendor::AMD;
            }
            pclose(pipe);
        }

        command = "lspci | grep -i 'vga\\|3d\\|2d' | grep -i 'intel'";
        pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                pclose(pipe);
                return GPUVendor::INTEL;
            }
            pclose(pipe);
        }

        return GPUVendor::UNKNOWN;
    }

    float getGPUTemperatureNVIDIA() {
        std::string command = "nvidia-smi --query-gpu=temperature.gpu --format=csv -i " + std::to_string(gpuIndex) + " | grep -o '[0-9]*'";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось выполнить nvidia-smi для получения температуры GPU." << std::endl;
            syslog(LOG_ERR, "Failed to execute nvidia-smi for GPU temperature");
            return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result);
            } catch (...) {
                std::cerr << "Ошибка: Не удалось преобразовать результат nvidia-smi в число." << std::endl;
                syslog(LOG_ERR, "Failed to convert nvidia-smi result to number");
                return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
            }
        }
        pclose(pipe);
        std::cerr << "Ошибка: Не удалось получить данные о температуре от nvidia-smi." << std::endl;
        syslog(LOG_ERR, "Failed to get temperature data from nvidia-smi");
        return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
    }

    float getGPUTemperatureAMD() {
        std::string path = "/sys/class/drm/card" + std::to_string(gpuIndex) + "/device/hwmon/hwmon*/temp1_input";
        std::string command = "cat " + path;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось получить температуру AMD GPU из sysfs." << std::endl;
            syslog(LOG_ERR, "Failed to get AMD GPU temperature from sysfs");
            return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result) / 1000.0f; // Конвертация из миллиградусов в градусы Цельсия
            } catch (...) {
                std::cerr << "Ошибка: Не удалось преобразовать результат sysfs в число." << std::endl;
                syslog(LOG_ERR, "Failed to convert sysfs result to number");
                return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
            }
        }
        pclose(pipe);
        std::cerr << "Ошибка: Не удалось получить данные о температуре из sysfs для AMD GPU." << std::endl;
        syslog(LOG_ERR, "Failed to get temperature data from sysfs for AMD GPU");
        return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
    }

    float getGPUTemperatureIntel() {
        // Для Intel GPU можно использовать sensors или другие утилиты
        std::string command = "sensors | grep -i 'temp' | grep -i 'package' | grep -o '[0-9]*\\.[0-9]*' | head -1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось получить температуру Intel GPU через sensors." << std::endl;
            syslog(LOG_ERR, "Failed to get Intel GPU temperature via sensors");
            return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result);
            } catch (...) {
                std::cerr << "Ошибка: Не удалось преобразовать результат sensors в число." << std::endl;
                syslog(LOG_ERR, "Failed to convert sensors result to number");
                return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
            }
        }
        pclose(pipe);
        std::cerr << "Ошибка: Не удалось получить данные о температуре от sensors для Intel GPU." << std::endl;
        syslog(LOG_ERR, "Failed to get temperature data from sensors for Intel GPU");
        return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
    }

    float getGPUTemperature() {
        switch (detectedVendor) {
            case GPUVendor::NVIDIA:
                return getGPUTemperatureNVIDIA();
            case GPUVendor::AMD:
                return getGPUTemperatureAMD();
            case GPUVendor::INTEL:
                return getGPUTemperatureIntel();
            default:
                std::cerr << "Неизвестный производитель GPU, температура недоступна." << std::endl;
                syslog(LOG_ERR, "Unknown GPU vendor, temperature unavailable");
                return currentTemperature; // Возвращаем предыдущее значение в случае ошибки
        }
    }

    float getGPULoadNVIDIA() {
        std::string command = "nvidia-smi --query-gpu=utilization.gpu --format=csv -i " + std::to_string(gpuIndex) + " | grep -o '[0-9]*'";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось выполнить nvidia-smi для получения нагрузки GPU." << std::endl;
            syslog(LOG_ERR, "Failed to execute nvidia-smi for GPU load");
            return currentLoad; // Возвращаем предыдущее значение в случае ошибки
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result);
            } catch (...) {
                std::cerr << "Ошибка: Не удалось преобразовать результат nvidia-smi в число для нагрузки." << std::endl;
                syslog(LOG_ERR, "Failed to convert nvidia-smi result to number for load");
                return currentLoad; // Возвращаем предыдущее значение в случае ошибки
            }
        }
        pclose(pipe);
        std::cerr << "Ошибка: Не удалось получить данные о нагрузке от nvidia-smi." << std::endl;
        syslog(LOG_ERR, "Failed to get load data from nvidia-smi");
        return currentLoad; // Возвращаем предыдущее значение в случае ошибки
    }

    float getGPULoadAMD() {
        // Для AMD можно использовать radeontop или другие утилиты, но это сложнее
        std::string command = "radeontop -d - -l 1 | grep -i 'gpu' | grep -o '[0-9]*\\.[0-9]*' | head -1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось получить нагрузку AMD GPU через radeontop." << std::endl;
            syslog(LOG_ERR, "Failed to get AMD GPU load via radeontop");
            return currentLoad; // Возвращаем предыдущее значение в случае ошибки
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result);
            } catch (...) {
                std::cerr << "Ошибка: Не удалось преобразовать результат radeontop в число." << std::endl;
                syslog(LOG_ERR, "Failed to convert radeontop result to number");
                return currentLoad; // Возвращаем предыдущее значение в случае ошибки
            }
        }
        pclose(pipe);
        std::cerr << "Ошибка: Не удалось получить данные о нагрузке от radeontop для AMD GPU." << std::endl;
        syslog(LOG_ERR, "Failed to get load data from radeontop for AMD GPU");
        return currentLoad; // Возвращаем предыдущее значение в случае ошибки
    }

    float getGPULoadIntel() {
        // Для Intel GPU нагрузка может быть получена через intel_gpu_top
        std::string command = "intel_gpu_top -l 1 | grep -i 'render' | grep -o '[0-9]*\\.[0-9]*' | head -1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось получить нагрузку Intel GPU через intel_gpu_top." << std::endl;
            syslog(LOG_ERR, "Failed to get Intel GPU load via intel_gpu_top");
            return currentLoad; // Возвращаем предыдущее значение в случае ошибки
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result);
            } catch (...) {
                std::cerr << "Ошибка: Не удалось преобразовать результат intel_gpu_top в число." << std::endl;
                syslog(LOG_ERR, "Failed to convert intel_gpu_top result to number");
                return currentLoad; // Возвращаем предыдущее значение в случае ошибки
            }
        }
        pclose(pipe);
        std::cerr << "Ошибка: Не удалось получить данные о нагрузке от intel_gpu_top для Intel GPU." << std::endl;
        syslog(LOG_ERR, "Failed to get load data from intel_gpu_top for Intel GPU");
        return currentLoad; // Возвращаем предыдущее значение в случае ошибки
    }

    float getGPULoad() {
        switch (detectedVendor) {
            case GPUVendor::NVIDIA:
                return getGPULoadNVIDIA();
            case GPUVendor::AMD:
                return getGPULoadAMD();
            case GPUVendor::INTEL:
                return getGPULoadIntel();
            default:
                std::cerr << "Неизвестный производитель GPU, нагрузка недоступна." << std::endl;
                syslog(LOG_ERR, "Unknown GPU vendor, load unavailable");
                return currentLoad; // Возвращаем предыдущее значение в случае ошибки
        }
    }

    bool setGPUPerformanceLevelNVIDIA(const std::string& level) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки уровня производительности NVIDIA GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set NVIDIA GPU performance level");
            return false;
        }
        int powerLimit;
        std::string clockCommand;
        if (level == "low") {
            powerLimit = 50;
            clockCommand = "nvidia-smi -ac 2000,800 -i " + std::to_string(gpuIndex); // Ограничение частот для снижения износа
        } else if (level == "high") {
            powerLimit = 100;
            clockCommand = "nvidia-smi -rac -i " + std::to_string(gpuIndex); // Сброс ограничений для максимальной производительности
        } else {
            powerLimit = 75; // auto или medium
            clockCommand = "nvidia-smi -ac 3000,1000 -i " + std::to_string(gpuIndex); // Средний уровень частот
        }
        std::string powerCommand = "nvidia-smi -pl " + std::to_string(powerLimit) + " -i " + std::to_string(gpuIndex);
        if (system(powerCommand.c_str()) == 0 && system(clockCommand.c_str()) == 0) {
            currentPerformanceLevel = level;
            currentPowerLimit = powerLimit;
            std::cout << "Уровень производительности NVIDIA GPU установлен на " << level << std::endl;
            syslog(LOG_INFO, "NVIDIA GPU performance level set to %s", level.c_str());
            return true;
        } else {
            std::cerr << "Ошибка при установке уровня производительности NVIDIA GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set NVIDIA GPU performance level");
            return false;
        }
    }

    bool setGPUPerformanceLevelAMD(const std::string& level) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки уровня производительности AMD GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set AMD GPU performance level");
            return false;
        }
        // Для AMD используем radeon-profile или corectrl
        std::string command;
        if (level == "low") {
            command = "radeon-profile --set-power-level low";
            currentPowerLimit = 50;
        } else if (level == "high") {
            command = "radeon-profile --set-power-level high";
            currentPowerLimit = 100;
        } else {
            command = "radeon-profile --set-power-level auto";
            currentPowerLimit = 75;
        }
        if (system(command.c_str()) == 0) {
            currentPerformanceLevel = level;
            std::cout << "Уровень производительности AMD GPU установлен на " << level << std::endl;
            syslog(LOG_INFO, "AMD GPU performance level set to %s", level.c_str());
            return true;
        } else {
            std::cerr << "Ошибка при установке уровня производительности AMD GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set AMD GPU performance level");
            return false;
        }
    }

    bool setGPUPerformanceLevelIntel(const std::string& level) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки уровня производительности Intel GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set Intel GPU performance level");
            return false;
        }
        // Для Intel можно использовать i915 driver параметры
        std::string command;
        if (level == "low") {
            command = "echo 0 | tee /sys/class/drm/card" + std::to_string(gpuIndex) + "/gt_min_freq_mhz";
            currentPowerLimit = 50;
        } else if (level == "high") {
            command = "echo 1 | tee /sys/class/drm/card" + std::to_string(gpuIndex) + "/gt_boost_freq_mhz";
            currentPowerLimit = 100;
        } else {
            command = "echo 0 | tee /sys/class/drm/card" + std::to_string(gpuIndex) + "/gt_boost_freq_mhz";
            currentPowerLimit = 75;
        }
        if (system(command.c_str()) == 0) {
            currentPerformanceLevel = level;
            std::cout << "Уровень производительности Intel GPU установлен на " << level << std::endl;
            syslog(LOG_INFO, "Intel GPU performance level set to %s", level.c_str());
            return true;
        } else {
            std::cerr << "Ошибка при установке уровня производительности Intel GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set Intel GPU performance level");
            return false;
        }
    }

    bool setGPUPowerLimitNVIDIA(int percent) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки лимита мощности NVIDIA GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set NVIDIA GPU power limit");
            return false;
        }
        // Преобразуем процент в ватты (предполагаем, что 100% = 300W)
        int powerLimitWatts = (percent * 300) / 100;
        std::string command = "nvidia-smi -pl " + std::to_string(powerLimitWatts) + " -i " + std::to_string(gpuIndex);
        if (system(command.c_str()) == 0) {
            currentPowerLimit = percent;
            // Обновляем уровень производительности в зависимости от лимита мощности
            if (percent <= 50) {
                currentPerformanceLevel = "low";
            } else if (percent >= 90) {
                currentPerformanceLevel = "high";
            } else {
                currentPerformanceLevel = "auto";
            }
            std::cout << "Лимит мощности NVIDIA GPU установлен на " << percent << "% (" << powerLimitWatts << "W)" << std::endl;
            syslog(LOG_INFO, "NVIDIA GPU power limit set to %d%% (%dW)", percent, powerLimitWatts);
            return true;
        } else {
            std::cerr << "Ошибка при установке лимита мощности NVIDIA GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set NVIDIA GPU power limit");
            return false;
        }
    }

    bool setGPUPowerLimitAMD(int percent) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки лимита мощности AMD GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set AMD GPU power limit");
            return false;
        }
        // Для AMD используем radeon-profile или corectrl
        std::string command;
        if (percent <= 50) {
            command = "radeon-profile --set-power-level low";
            currentPerformanceLevel = "low";
        } else if (percent >= 90) {
            command = "radeon-profile --set-power-level high";
            currentPerformanceLevel = "high";
        } else {
            command = "radeon-profile --set-power-level auto";
            currentPerformanceLevel = "auto";
        }
        if (system(command.c_str()) == 0) {
            currentPowerLimit = percent;
            std::cout << "Лимит мощности AMD GPU установлен на " << percent << "%" << std::endl;
            syslog(LOG_INFO, "AMD GPU power limit set to %d%%", percent);
            return true;
        } else {
            std::cerr << "Ошибка при установке лимита мощности AMD GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set AMD GPU power limit");
            return false;
        }
    }

    bool setGPUPowerLimitIntel(int percent) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки лимита мощности Intel GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set Intel GPU power limit");
            return false;
        }
        // Для Intel можно использовать i915 driver параметры
        std::string command;
        if (percent <= 50) {
            command = "echo 0 | tee /sys/class/drm/card" + std::to_string(gpuIndex) + "/gt_min_freq_mhz";
            currentPerformanceLevel = "low";
        } else if (percent >= 90) {
            command = "echo 1 | tee /sys/class/drm/card" + std::to_string(gpuIndex) + "/gt_boost_freq_mhz";
            currentPerformanceLevel = "high";
        } else {
            command = "echo 0 | tee /sys/class/drm/card" + std::to_string(gpuIndex) + "/gt_boost_freq_mhz";
            currentPerformanceLevel = "auto";
        }
        if (system(command.c_str()) == 0) {
            currentPowerLimit = percent;
            std::cout << "Лимит мощности Intel GPU установлен на " << percent << "%" << std::endl;
            syslog(LOG_INFO, "Intel GPU power limit set to %d%%", percent);
            return true;
        } else {
            std::cerr << "Ошибка при установке лимита мощности Intel GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set Intel GPU power limit");
            return false;
        }
    }

    bool setGPUFanSpeedNVIDIA(int percent) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки скорости вентилятора NVIDIA GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set NVIDIA GPU fan speed");
            return false;
        }
        // Включаем ручное управление вентилятором
        std::string manualCommand = "nvidia-settings -a [gpu:" + std::to_string(gpuIndex) + "]/GPUFanControlState=1";
        // Устанавливаем скорость вентилятора
        std::string fanCommand = "nvidia-settings -a [fan:" + std::to_string(gpuIndex) + "]/GPUTargetFanSpeed=" + std::to_string(percent);
        if (system(manualCommand.c_str()) == 0 && system(fanCommand.c_str()) == 0) {
            currentFanSpeed = percent;
            std::cout << "Скорость вентилятора NVIDIA GPU установлена на " << percent << "%" << std::endl;
            syslog(LOG_INFO, "NVIDIA GPU fan speed set to %d%%", percent);
            return true;
        } else {
            std::cerr << "Ошибка при установке скорости вентилятора NVIDIA GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set NVIDIA GPU fan speed");
            return false;
        }
    }

    bool setGPUFanSpeedAMD(int percent) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки скорости вентилятора AMD GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set AMD GPU fan speed");
            return false;
        }
        // Для AMD используем radeon-profile или corectrl
        std::string command = "radeon-profile --set-fan-speed " + std::to_string(percent);
        if (system(command.c_str()) == 0) {
            currentFanSpeed = percent;
            std::cout << "Скорость вентилятора AMD GPU установлена на " << percent << "%" << std::endl;
            syslog(LOG_INFO, "AMD GPU fan speed set to %d%%", percent);
            return true;
        } else {
            std::cerr << "Ошибка при установке скорости вентилятора AMD GPU" << std::endl;
            syslog(LOG_ERR, "Failed to set AMD GPU fan speed");
            return false;
        }
    }

    bool setGPUFanSpeedIntel(int percent) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для установки скорости вентилятора Intel GPU." << std::endl;
            syslog(LOG_ERR, "Root privileges required to set Intel GPU fan speed");
            return false;
        }
        // Для Intel управление вентилятором может быть недоступно напрямую
        std::cerr << "Предупреждение: Управление скоростью вентилятора Intel GPU не поддерживается." << std::endl;
        syslog(LOG_WARNING, "Intel GPU fan speed control not supported");
        return false;
    }

    void autoOptimize() {
        while (autoOptimizationEnabled) {
            currentTemperature = getGPUTemperature();
            currentLoad = getGPULoad();

            // Улучшенная автоматическая настройка уровня производительности и лимита мощности
            if (currentTemperature > 85.0f && currentPerformanceLevel != "low") {
                SetPerformanceLevel("low");
                std::cout << "Автооптимизация: Уровень производительности GPU снижен до low из-за критически высокой температуры " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU performance level set to low due to critical temperature %.2fC", currentTemperature);
            } else if (currentTemperature < 45.0f && currentLoad > 85.0f && currentPerformanceLevel != "high") {
                SetPerformanceLevel("high");
                std::cout << "Автооптимизация: Уровень производительности GPU повышен до high из-за высокой нагрузки " << currentLoad << "% и низкой температуры " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU performance level set to high due to high load %.2f%% and low temperature %.2fC", currentLoad, currentTemperature);
            } else if ((currentTemperature >= 45.0f && currentTemperature <= 85.0f) && (currentLoad >= 25.0f && currentLoad <= 85.0f) && currentPerformanceLevel != "auto") {
                SetPerformanceLevel("auto");
                std::cout << "Автооптимизация: Уровень производительности GPU установлен на auto из-за умеренной температуры " << currentTemperature << "C и нагрузки " << currentLoad << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU performance level set to auto due to moderate temperature %.2fC and load %.2f%%", currentTemperature, currentLoad);
            }

            // Улучшенная автоматическая настройка скорости вентилятора
            if (currentTemperature > 80.0f && currentFanSpeed < 90) {
                SetFanSpeed(90);
                std::cout << "Автооптимизация: Скорость вентилятора GPU увеличена до 90% из-за критически высокой температуры " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU fan speed increased to 90%% due to critical temperature %.2fC", currentTemperature);
            } else if (currentTemperature < 45.0f && currentFanSpeed > 30) {
                SetFanSpeed(30);
                std::cout << "Автооптимизация: Скорость вентилятора GPU снижена до 30% из-за низкой температуры " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU fan speed decreased to 30%% due to low temperature %.2fC", currentTemperature);
            } else if (currentTemperature >= 45.0f && currentTemperature <= 80.0f && (currentFanSpeed < 45 || currentFanSpeed > 65)) {
                SetFanSpeed(55);
                std::cout << "Автооптимизация: Скорость вентилятора GPU установлена на 55% из-за умеренной температуры " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU fan speed set to 55%% due to moderate temperature %.2fC", currentTemperature);
            }

            // Дополнительная защита от износа: снижение производительности при экстремальной температуре
            if (currentTemperature > 90.0f && currentPerformanceLevel != "low") {
                SetPerformanceLevel("low");
                SetPowerLimit(40);
                std::cout << "Автооптимизация: Уровень производительности GPU снижен до low и лимит мощности до 40% из-за экстремальной температуры " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU performance level set to low and power limit to 40%% due to extreme temperature %.2fC", currentTemperature);
            }

            // Дополнительное логирование для отслеживания изменений
            static float lastTemperature = currentTemperature;
            static float lastLoad = currentLoad;
            static std::string lastPerformanceLevel = currentPerformanceLevel;
            static int lastFanSpeed = currentFanSpeed;
            static int lastPowerLimit = currentPowerLimit;

            if (std::abs(lastTemperature - currentTemperature) > 5.0f) {
                std::cout << "Автооптимизация: Температура GPU изменилась с " << lastTemperature << "C на " << currentTemperature << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU temperature changed from %.2fC to %.2fC", lastTemperature, currentTemperature);
                lastTemperature = currentTemperature;
            }
            if (std::abs(lastLoad - currentLoad) > 10.0f) {
                std::cout << "Автооптимизация: Нагрузка GPU изменилась с " << lastLoad << "% на " << currentLoad << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU load changed from %.2f%% to %.2f%%", lastLoad, currentLoad);
                lastLoad = currentLoad;
            }
            if (lastPerformanceLevel != currentPerformanceLevel) {
                std::cout << "Автооптимизация: Уровень производительности GPU изменился с " << lastPerformanceLevel << " на " << currentPerformanceLevel << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU performance level changed from %s to %s", lastPerformanceLevel.c_str(), currentPerformanceLevel.c_str());
                lastPerformanceLevel = currentPerformanceLevel;
            }
            if (lastFanSpeed != currentFanSpeed) {
                std::cout << "Автооптимизация: Скорость вентилятора GPU изменилась с " << lastFanSpeed << "% на " << currentFanSpeed << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU fan speed changed from %d%% to %d%%", lastFanSpeed, currentFanSpeed);
                lastFanSpeed = currentFanSpeed;
            }
            if (lastPowerLimit != currentPowerLimit) {
                std::cout << "Автооптимизация: Лимит мощности GPU изменился с " << lastPowerLimit << "% на " << currentPowerLimit << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: GPU power limit changed from %d%% to %d%%", lastPowerLimit, currentPowerLimit);
                lastPowerLimit = currentPowerLimit;
            }

            std::this_thread::sleep_for(std::chrono::seconds(15)); // Проверка каждые 15 секунд для снижения нагрузки
        }
    }

public:
    GPUOptimizer() : isInitialized(false), currentPerformanceLevel("auto"), currentPowerLimit(75), currentFanSpeed(50), autoOptimizationEnabled(false), currentTemperature(0.0f), currentLoad(0.0f), detectedVendor(GPUVendor::UNKNOWN), gpuIndex(0), isRoot(false) {
        std::cout << "Конструктор GPUOptimizer вызван" << std::endl;
        openlog("OptiCoreGPUOptimizer", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "GPUOptimizer constructor called");
    }

    ~GPUOptimizer() {
        std::cout << "Деструктор GPUOptimizer вызван" << std::endl;
        autoOptimizationEnabled = false; // Остановить автооптимизацию
        syslog(LOG_INFO, "GPUOptimizer destructor called");
        closelog();
    }

    bool Initialize() override {
        std::cout << "Инициализация GPUOptimizer" << std::endl;
        isInitialized = true;
        isRoot = checkRoot();
        if (!isRoot) {
            std::cerr << "Предупреждение: Программа запущена без прав root, некоторые функции оптимизации GPU могут быть недоступны." << std::endl;
            syslog(LOG_WARNING, "Program running without root privileges, some GPU optimization features may be unavailable");
        }
        detectedVendor = detectGPUVendor();
        if (detectedVendor == GPUVendor::UNKNOWN) {
            std::cerr << "Предупреждение: Не удалось определить производителя GPU, некоторые функции могут быть недоступны." << std::endl;
            syslog(LOG_WARNING, "Failed to detect GPU vendor, some features may be unavailable");
        } else {
            std::string vendorName = (detectedVendor == GPUVendor::NVIDIA ? "NVIDIA" : (detectedVendor == GPUVendor::AMD ? "AMD" : "Intel"));
            std::cout << "Обнаружен GPU производителя: " << vendorName << std::endl;
            syslog(LOG_INFO, "Detected GPU vendor: %s", vendorName.c_str());
        }
        currentTemperature = getGPUTemperature();
        currentLoad = getGPULoad();
        std::cout << "GPUOptimizer успешно инициализирован" << std::endl;
        syslog(LOG_INFO, "GPUOptimizer initialized successfully");
        return true;
    }

    bool SetPerformanceLevel(const std::string& level) override {
        std::cout << "Установка уровня производительности GPU на " << level << std::endl;
        if (!isInitialized) {
            std::cerr << "GPUOptimizer не инициализирован при попытке установки уровня производительности" << std::endl;
            syslog(LOG_ERR, "GPUOptimizer not initialized when attempting to set performance level");
            return false;
        }
        if (level != "low" && level != "auto" && level != "high") {
            std::cerr << "Ошибка: Неверный уровень производительности, должен быть low, auto или high" << std::endl;
            syslog(LOG_ERR, "Invalid performance level, must be low, auto, or high");
            return false;
        }
        switch (detectedVendor) {
            case GPUVendor::NVIDIA:
                return setGPUPerformanceLevelNVIDIA(level);
            case GPUVendor::AMD:
                return setGPUPerformanceLevelAMD(level);
            case GPUVendor::INTEL:
                return setGPUPerformanceLevelIntel(level);
            default:
                std::cerr << "Неизвестный производитель GPU, установка уровня производительности невозможна." << std::endl;
                syslog(LOG_ERR, "Unknown GPU vendor, setting performance level not possible");
                return false;
        }
    }

    bool SetPowerLimit(int percent) override {
        std::cout << "Установка лимита мощности GPU на " << percent << "%" << std::endl;
        if (!isInitialized) {
            std::cerr << "GPUOptimizer не инициализирован при попытке установки лимита мощности" << std::endl;
            syslog(LOG_ERR, "GPUOptimizer not initialized when attempting to set power limit");
            return false;
        }
        if (percent < 10 || percent > 120) {
            std::cerr << "Ошибка: Лимит мощности должен быть в диапазоне от 10% до 120%" << std::endl;
            syslog(LOG_ERR, "Power limit must be between 10%% and 120%%");
            return false;
        }
        switch (detectedVendor) {
            case GPUVendor::NVIDIA:
                return setGPUPowerLimitNVIDIA(percent);
            case GPUVendor::AMD:
                return setGPUPowerLimitAMD(percent);
            case GPUVendor::INTEL:
                return setGPUPowerLimitIntel(percent);
            default:
                std::cerr << "Неизвестный производитель GPU, установка лимита мощности невозможна." << std::endl;
                syslog(LOG_ERR, "Unknown GPU vendor, setting power limit not possible");
                return false;
        }
    }

    bool SetFanSpeed(int percent) override {
        std::cout << "Установка скорости вентилятора GPU на " << percent << "%" << std::endl;
        if (!isInitialized) {
            std::cerr << "GPUOptimizer не инициализирован при попытке установки скорости вентилятора" << std::endl;
            syslog(LOG_ERR, "GPUOptimizer not initialized when attempting to set fan speed");
            return false;
        }
        if (percent < 0 || percent > 100) {
            std::cerr << "Ошибка: Скорость вентилятора должна быть в диапазоне от 0% до 100%" << std::endl;
            syslog(LOG_ERR, "Fan speed must be between 0%% and 100%%");
            return false;
        }
        switch (detectedVendor) {
            case GPUVendor::NVIDIA:
                return setGPUFanSpeedNVIDIA(percent);
            case GPUVendor::AMD:
                return setGPUFanSpeedAMD(percent);
            case GPUVendor::INTEL:
                return setGPUFanSpeedIntel(percent);
            default:
                std::cerr << "Неизвестный производитель GPU, установка скорости вентилятора невозможна." << std::endl;
                syslog(LOG_ERR, "Unknown GPU vendor, setting fan speed not possible");
                return false;
        }
    }

    bool EnableAutoOptimization(bool enable) override {
        std::cout << "Установка автооптимизации GPU: " << (enable ? "включено" : "выключено") << std::endl;
        if (!isInitialized) {
            std::cerr << "GPUOptimizer не инициализирован при попытке установки автооптимизации" << std::endl;
            syslog(LOG_ERR, "GPUOptimizer not initialized when attempting to set auto optimization");
            return false;
        }
        autoOptimizationEnabled = enable;
        if (enable) {
            std::thread(&GPUOptimizer::autoOptimize, this).detach();
        }
        std::cout << "Автооптимизация GPU " << (enable ? "включена" : "выключена") << std::endl;
        syslog(LOG_INFO, "GPU auto optimization %s", enable ? "enabled" : "disabled");
        return true;
    }

    std::string QueryStatus() override {
        std::cout << "Получение статуса GPUOptimizer" << std::endl;
        syslog(LOG_INFO, "Getting GPUOptimizer status");
        currentTemperature = getGPUTemperature();
        currentLoad = getGPULoad();
        std::string vendorName = (detectedVendor == GPUVendor::NVIDIA ? "NVIDIA" : (detectedVendor == GPUVendor::AMD ? "AMD" : (detectedVendor == GPUVendor::INTEL ? "Intel" : "Unknown")));
        return "GPUOptimizer: Initialized=" + std::string(isInitialized ? "true" : "false") +
               ", RootPrivileges=" + std::string(isRoot ? "true" : "false") +
               ", Vendor=" + vendorName +
               ", PerformanceLevel=" + currentPerformanceLevel +
               ", PowerLimit=" + std::to_string(currentPowerLimit) + "%" +
               ", FanSpeed=" + std::to_string(currentFanSpeed) + "%" +
               ", CurrentTemperature=" + std::to_string(currentTemperature) + "C" +
               ", CurrentLoad=" + std::to_string(currentLoad) + "%" +
               ", AutoOptimization=" + std::string(autoOptimizationEnabled ? "enabled" : "disabled");
    }
};

// Экспортируемая функция для создания экземпляра
extern "C" IGPUOptimizer* CreateGPUOptimizer() {
    std::cout << "Создание экземпляра GPUOptimizer" << std::endl;
    syslog(LOG_INFO, "Creating GPUOptimizer instance");
    return new GPUOptimizer();
}
