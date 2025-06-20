#include "analytics.h"
#include <iostream>
#include <string>
#include <syslog.h>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>

class Analytics : public IAnalytics {
private:
    bool isInitialized;
    bool running;
    AnalyticsReport lastReport;
    float cpuLoadSum;
    float memoryUsageSum;
    float gpuLoadSum;
    int sampleCount;

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

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

public:
    Analytics() : isInitialized(false), running(false), cpuLoadSum(0.0f), memoryUsageSum(0.0f), gpuLoadSum(0.0f), sampleCount(0) {
        std::cout << "Конструктор Analytics вызван" << std::endl;
        openlog("OptiCoreAnalytics", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "Analytics constructor called");
        // Инициализация отчета начальными значениями
        lastReport.type = "none";
        lastReport.efficiency = 0.0f;
        lastReport.timestamp = "none";
    }

    ~Analytics() {
        std::cout << "Деструктор Analytics вызван" << std::endl;
        syslog(LOG_INFO, "Analytics destructor called");
        closelog();
    }

    bool Initialize() override {
        std::cout << "Инициализация Analytics" << std::endl;
        isInitialized = true;
        std::cout << "Analytics успешно инициализирована" << std::endl;
        syslog(LOG_INFO, "Analytics initialized successfully");
        return true;
    }

    bool RunAnalysis(const std::string& type, int duration_sec) override {
        std::cout << "Запуск анализа типа " << type << " на " << duration_sec << " секунд" << std::endl;
        if (!isInitialized) {
            std::cerr << "Analytics не инициализирована при попытке запуска анализа" << std::endl;
            syslog(LOG_ERR, "Analytics not initialized when attempting to run analysis");
            return false;
        }
        running = true;
        cpuLoadSum = 0.0f;
        memoryUsageSum = 0.0f;
        gpuLoadSum = 0.0f;
        sampleCount = 0;
        std::cout << "Анализ типа " << type << " запущен" << std::endl;
        syslog(LOG_INFO, "Analysis of type %s started", type.c_str());
        std::thread([this, type, duration_sec]() {
            auto startTime = std::chrono::system_clock::now();
            while (running && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTime).count() < duration_sec) {
                float cpuLoad = getCpuLoad();
                float memoryUsage = getMemoryUsage();
                float gpuLoad = getGpuLoad();
                cpuLoadSum += cpuLoad;
                memoryUsageSum += memoryUsage;
                gpuLoadSum += gpuLoad;
                sampleCount++;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (sampleCount > 0) {
                float avgCpuLoad = cpuLoadSum / sampleCount;
                float avgMemoryUsage = memoryUsageSum / sampleCount;
                float avgGpuLoad = gpuLoadSum / sampleCount;
                // Простая формула для эффективности: среднее значение нагрузки CPU, GPU и памяти
                lastReport.efficiency = (100.0f - (avgCpuLoad + avgMemoryUsage + avgGpuLoad) / 3.0f);
                if (lastReport.efficiency < 0.0f) lastReport.efficiency = 0.0f;
            } else {
                lastReport.efficiency = 0.0f;
            }
            lastReport.type = type;
            lastReport.timestamp = getCurrentTimestamp();
            std::cout << "Анализ типа " << type << " завершен с эффективностью " << lastReport.efficiency << "%" << std::endl;
            syslog(LOG_INFO, "Analysis of type %s completed with efficiency %f%%", type.c_str(), lastReport.efficiency);
            running = false;
        }).detach();
        return true;
    }

    bool IsAnalysisRunning() override {
        std::cout << "Проверка состояния анализа" << std::endl;
        syslog(LOG_INFO, "Checking analysis status");
        return running;
    }

    virtual AnalyticsReport GetLastReport() {
        std::cout << "Получение последнего отчета Analytics" << std::endl;
        syslog(LOG_INFO, "Getting last Analytics report");
        return lastReport;
    }

    virtual std::string QueryStatus() {
        std::cout << "Получение статуса Analytics" << std::endl;
        syslog(LOG_INFO, "Getting Analytics status");
        return "Analytics: Initialized=" + std::string(isInitialized ? "true" : "false") +
               ", Running=" + std::string(running ? "true" : "false") +
               ", LastReportType=" + lastReport.type;
    }
};

// Экспортируемая функция для создания экземпляра
extern "C" IAnalytics* CreateAnalytics() {
    std::cout << "Создание экземпляра Analytics" << std::endl;
    syslog(LOG_INFO, "Creating Analytics instance");
    return new Analytics();
}
