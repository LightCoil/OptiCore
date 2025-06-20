#include "monitor.h"
#include <iostream>
#include <string>
#include <syslog.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <cstdlib>

class Monitor : public IMonitor {
private:
    bool isInitialized;
    bool isMonitoring;
    MonitorMetrics currentMetrics;

    float getCpuLoad() {
        std::ifstream statFile("/proc/stat");
        if (!statFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/stat для расчета нагрузки CPU." << std::endl;
            return 0.0f;
        }

        std::string line;
        std::getline(statFile, line);
        std::istringstream iss(line);
        std::string cpu;
        long user, nice, system, idle;
        iss >> cpu >> user >> nice >> system >> idle;

        static long prevIdle = 0, prevTotal = 0;
        long total = user + nice + system + idle;
        long totalDiff = total - prevTotal;
        long idleDiff = idle - prevIdle;

        float load = totalDiff > 0 ? (float)(totalDiff - idleDiff) / totalDiff * 100.0f : 0.0f;

        prevIdle = idle;
        prevTotal = total;

        return load;
    }

    float getMemoryUsage() {
        std::ifstream memFile("/proc/meminfo");
        if (!memFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/meminfo для расчета использования памяти." << std::endl;
            return 0.0f;
        }

        std::string line;
        long total = 0, free = 0;
        while (std::getline(memFile, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> total;
            } else if (line.find("MemFree:") != std::string::npos) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> free;
            }
        }

        if (total > 0) {
            return (float)(total - free) / total * 100.0f;
        }
        return 0.0f;
    }

    float getGpuLoad() {
        // Для NVIDIA GPU можно использовать nvidia-smi
        std::string command = "nvidia-smi --query-gpu=utilization.gpu --format=csv -i 0 | grep -o '[0-9]*'";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Ошибка: Не удалось выполнить nvidia-smi для получения нагрузки GPU." << std::endl;
            return 0.0f;
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                return std::stof(result);
            } catch (...) {
                return 0.0f;
            }
        }
        pclose(pipe);
        return 0.0f;
    }

    float getPowerConsumption() {
        // Чтение данных о потреблении энергии из /sys/class/power_supply
        std::ifstream powerFile("/sys/class/power_supply/BAT0/power_now");
        if (powerFile.is_open()) {
            float power;
            if (powerFile >> power) {
                return power / 1000000.0f; // Конвертация из микроватт в ватты
            }
        }
        // Если данные недоступны, используем альтернативный метод или возвращаем среднее значение
        std::ifstream currentFile("/sys/class/power_supply/BAT0/current_now");
        std::ifstream voltageFile("/sys/class/power_supply/BAT0/voltage_now");
        if (currentFile.is_open() && voltageFile.is_open()) {
            float current, voltage;
            if (currentFile >> current && voltageFile >> voltage) {
                return (current * voltage) / 1000000000000.0f; // Конвертация в ватты
            }
        }
        std::cerr << "Предупреждение: Данные о потреблении энергии недоступны, возвращается среднее значение." << std::endl;
        syslog(LOG_WARNING, "Power consumption data unavailable, returning average value");
        return 25.0f; // Среднее значение, если данные недоступны
    }

    float getIoOperations() {
        std::ifstream ioFile("/proc/diskstats");
        if (!ioFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/diskstats для расчета операций ввода-вывода." << std::endl;
            return 0.0f;
        }

        std::string line;
        long totalOps = 0;
        while (std::getline(ioFile, line)) {
            std::istringstream iss(line);
            std::string dummy;
            long reads, writes;
            iss >> dummy >> dummy >> dummy >> reads >> dummy >> dummy >> dummy >> writes;
            totalOps += reads + writes;
        }

        static long prevOps = 0;
        float opsDiff = totalOps - prevOps;
        prevOps = totalOps;

        return opsDiff;
    }

    float getAudioQuality() {
        // Попытка получить данные о качестве аудио через PulseAudio
        std::string command = "pactl list sinks | grep 'Latency' | awk '{print $2}'";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Предупреждение: Не удалось выполнить pactl для получения данных о качестве аудио." << std::endl;
            syslog(LOG_WARNING, "Failed to execute pactl for audio quality data");
            return 70.0f; // Среднее значение, если данные недоступны
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result(buffer);
            pclose(pipe);
            try {
                float latency = std::stof(result);
                // Преобразуем задержку в оценку качества (меньше задержка - лучше качество)
                if (latency < 10000) return 90.0f; // Низкая задержка, высокое качество
                else if (latency < 50000) return 70.0f; // Средняя задержка, среднее качество
                else return 50.0f; // Высокая задержка, низкое качество
            } catch (...) {
                pclose(pipe);
                return 70.0f; // Среднее значение, если преобразование не удалось
            }
        }
        pclose(pipe);
        return 70.0f; // Среднее значение, если данные недоступны
    }

public:
    Monitor() : isInitialized(false), isMonitoring(false) {
        std::cout << "Конструктор Monitor вызван" << std::endl;
        openlog("OptiCoreMonitor", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "Monitor constructor called");
        // Инициализация метрик начальными значениями
        currentMetrics.cpuLoad = 0.0f;
        currentMetrics.memoryUsage = 0.0f;
        currentMetrics.gpuLoad = 0.0f;
        currentMetrics.powerConsumption = 0.0f;
        currentMetrics.ioOperations = 0.0f;
        currentMetrics.audioQuality = 0.0f;
    }

    ~Monitor() {
        std::cout << "Деструктор Monitor вызван" << std::endl;
        syslog(LOG_INFO, "Monitor destructor called");
        closelog();
    }

    bool Initialize() override {
        std::cout << "Инициализация Monitor" << std::endl;
        isInitialized = true;
        std::cout << "Monitor успешно инициализирован" << std::endl;
        syslog(LOG_INFO, "Monitor initialized successfully");
        return true;
    }

    bool StartMonitoring() override {
        std::cout << "Запуск мониторинга" << std::endl;
        if (!isInitialized) {
            std::cerr << "Monitor не инициализирован при попытке запуска мониторинга" << std::endl;
            syslog(LOG_ERR, "Monitor not initialized when attempting to start monitoring");
            return false;
        }
        isMonitoring = true;
        std::cout << "Мониторинг запущен" << std::endl;
        syslog(LOG_INFO, "Monitoring started");
        std::thread([this]() {
            while (isMonitoring) {
                currentMetrics.cpuLoad = getCpuLoad();
                currentMetrics.memoryUsage = getMemoryUsage();
                currentMetrics.gpuLoad = getGpuLoad();
                currentMetrics.powerConsumption = getPowerConsumption();
                currentMetrics.ioOperations = getIoOperations();
                currentMetrics.audioQuality = getAudioQuality();
                std::cout << "Метрики обновлены" << std::endl;
                syslog(LOG_INFO, "Metrics updated");
                std::this_thread::sleep_for(std::chrono::seconds(5)); // Обновление каждые 5 секунд
            }
        }).detach();
        return true;
    }

    bool StopMonitoring() override {
        std::cout << "Остановка мониторинга" << std::endl;
        if (!isInitialized) {
            std::cerr << "Monitor не инициализирован при попытке остановки мониторинга" << std::endl;
            syslog(LOG_ERR, "Monitor not initialized when attempting to stop monitoring");
            return false;
        }
        isMonitoring = false;
        std::cout << "Мониторинг остановлен" << std::endl;
        syslog(LOG_INFO, "Monitoring stopped");
        return true;
    }

    virtual MonitorMetrics GetCurrentMetrics() {
        std::cout << "Получение текущих метрик Monitor" << std::endl;
        syslog(LOG_INFO, "Getting current Monitor metrics");
        return currentMetrics;
    }
};

// Экспортируемая функция для создания экземпляра
extern "C" IMonitor* CreateMonitor() {
    std::cout << "Создание экземпляра Monitor" << std::endl;
    syslog(LOG_INFO, "Creating Monitor instance");
    return new Monitor();
}
