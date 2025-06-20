#include "cpu_optimizer.h"
#include <iostream>
#include <string>
#include <syslog.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

struct load_predictor_t {
    bool enabled;
    float predicted_load;
    float historical_loads[10]; // Массив для хранения последних 10 значений нагрузки
    int current_index;

    load_predictor_t() : enabled(false), predicted_load(0.0f), current_index(0) {
        std::cout << "Конструктор load_predictor_t вызван" << std::endl;
        for (int i = 0; i < 10; ++i) {
            historical_loads[i] = 0.0f;
        }
    }
};

struct temperature_monitor_t {
    bool enabled;
    float current_temperature;
    float critical_threshold;
    float warning_threshold;

    temperature_monitor_t() : enabled(false), current_temperature(0.0f), critical_threshold(85.0f), warning_threshold(70.0f) {
        std::cout << "Конструктор temperature_monitor_t вызван" << std::endl;
    }
};

class CpuOptimizer : public ICpuOptimizer {
private:
    bool isInitialized;
    load_predictor_t predictor;
    temperature_monitor_t temp_monitor;
    std::string currentGovernor;
    int currentFrequency; // Текущая частота в kHz
    bool autoOptimizationEnabled;
    float currentLoad;
    bool isRoot;

    bool checkRoot() {
        return geteuid() == 0;
    }

    bool writeToFile(const std::string& path, const std::string& value) {
        if (!isRoot) {
            std::cerr << "Ошибка: Требуются права root для записи в " << path << std::endl;
            syslog(LOG_ERR, "Root privileges required to write to %s", path.c_str());
            return false;
        }
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть файл " << path << " для записи." << std::endl;
            syslog(LOG_ERR, "Failed to open file %s for writing", path.c_str());
            return false;
        }
        file << value;
        if (file.fail()) {
            std::cerr << "Ошибка: Не удалось записать значение в файл " << path << std::endl;
            syslog(LOG_ERR, "Failed to write value to file %s", path.c_str());
            return false;
        }
        return true;
    }

    float calculateCpuLoad() {
        std::ifstream statFile("/proc/stat");
        if (!statFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/stat для расчета нагрузки CPU." << std::endl;
            syslog(LOG_ERR, "Failed to open /proc/stat for CPU load calculation");
            return currentLoad; // Возвращаем предыдущее значение в случае ошибки
        }

        std::string line;
        if (!std::getline(statFile, line)) {
            std::cerr << "Ошибка: Не удалось прочитать данные из /proc/stat." << std::endl;
            syslog(LOG_ERR, "Failed to read data from /proc/stat");
            return currentLoad; // Возвращаем предыдущее значение в случае ошибки
        }

        std::istringstream iss(line);
        std::string cpu;
        long user, nice, system, idle;
        iss >> cpu >> user >> nice >> system >> idle;

        static long prevIdle = 0, prevTotal = 0;
        long total = user + nice + system + idle;
        long totalDiff = total - prevTotal;
        long idleDiff = idle - prevIdle;

        float load = totalDiff > 0 ? (float)(totalDiff - idleDiff) / totalDiff * 100.0f : currentLoad;

        prevIdle = idle;
        prevTotal = total;

        if (predictor.enabled) {
            predictor.historical_loads[predictor.current_index] = load;
            predictor.current_index = (predictor.current_index + 1) % 10;
            // Улучшенное предсказание с учетом тренда и весов
            float sum = 0.0f;
            float weight_sum = 0.0f;
            int count = 0;
            float trend = 0.0f;
            for (int i = 0; i < 10; ++i) {
                if (predictor.historical_loads[i] > 0.0f) {
                    float weight = 1.0f + (i * 0.1f); // Более новые значения имеют больший вес
                    sum += predictor.historical_loads[i] * weight;
                    weight_sum += weight;
                    count++;
                    if (i > 0 && predictor.historical_loads[i - 1] > 0.0f) {
                        trend += (predictor.historical_loads[i] - predictor.historical_loads[i - 1]) * weight;
                    }
                }
            }
            trend = count > 1 ? trend / (count - 1) : 0.0f;
            predictor.predicted_load = count > 0 ? (sum / weight_sum) + trend * 0.5f : load;
            // Ограничение предсказанной нагрузки в разумных пределах
            if (predictor.predicted_load < 0.0f) predictor.predicted_load = 0.0f;
            if (predictor.predicted_load > 100.0f) predictor.predicted_load = 100.0f;
        }

        currentLoad = load;
        return load;
    }

    float getCpuTemperature() {
        std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
        if (!tempFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /sys/class/thermal/thermal_zone0/temp для чтения температуры." << std::endl;
            syslog(LOG_ERR, "Failed to open /sys/class/thermal/thermal_zone0/temp for temperature reading");
            return temp_monitor.current_temperature; // Возвращаем предыдущее значение в случае ошибки
        }

        float temp;
        if (!(tempFile >> temp)) {
            std::cerr << "Ошибка: Не удалось прочитать температуру из /sys/class/thermal/thermal_zone0/temp." << std::endl;
            syslog(LOG_ERR, "Failed to read temperature from /sys/class/thermal/thermal_zone0/temp");
            return temp_monitor.current_temperature; // Возвращаем предыдущее значение в случае ошибки
        }
        return temp / 1000.0f; // Конвертация из миллиградусов в градусы Цельсия
    }

    int getCurrentCpuFrequency() {
        std::ifstream freqFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        if (!freqFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq для чтения текущей частоты." << std::endl;
            syslog(LOG_ERR, "Failed to open /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq for current frequency");
            return currentFrequency; // Возвращаем предыдущее значение в случае ошибки
        }

        int freq;
        if (!(freqFile >> freq)) {
            std::cerr << "Ошибка: Не удалось прочитать текущую частоту из /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq." << std::endl;
            syslog(LOG_ERR, "Failed to read current frequency from /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
            return currentFrequency; // Возвращаем предыдущее значение в случае ошибки
        }
        return freq;
    }

    std::vector<int> getAvailableCpuFrequencies() {
        std::vector<int> frequencies;
        std::ifstream freqFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
        if (!freqFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies для чтения доступных частот." << std::endl;
            syslog(LOG_ERR, "Failed to open /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies for available frequencies");
            return frequencies;
        }

        std::string line;
        if (!std::getline(freqFile, line)) {
            std::cerr << "Ошибка: Не удалось прочитать доступные частоты из /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies." << std::endl;
            syslog(LOG_ERR, "Failed to read available frequencies from /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
            return frequencies;
        }
        std::istringstream iss(line);
        int freq;
        while (iss >> freq) {
            frequencies.push_back(freq);
        }
        std::sort(frequencies.begin(), frequencies.end(), std::greater<int>());
        return frequencies;
    }

    void autoOptimize() {
        while (autoOptimizationEnabled) {
            float currentLoad = calculateCpuLoad();
            float currentTemp = getCpuTemperature();
            temp_monitor.current_temperature = currentTemp;

            // Улучшенная автоматическая настройка governor на основе нагрузки и температуры
            if (currentTemp > temp_monitor.critical_threshold) {
                if (currentGovernor != "powersave") {
                    SetCpuGovernor("powersave");
                    std::cout << "Автооптимизация: Переключение на governor powersave из-за критической температуры " << currentTemp << "C" << std::endl;
                    syslog(LOG_INFO, "Auto-optimization: Switched to powersave governor due to critical temperature %.2fC", currentTemp);
                }
                // Дополнительно снижаем частоту до минимальной доступной
                std::vector<int> freqs = getAvailableCpuFrequencies();
                if (!freqs.empty() && currentFrequency != freqs.back()) {
                    SetCpuFrequency(freqs.back());
                    std::cout << "Автооптимизация: Частота снижена до минимальной " << freqs.back() << " kHz из-за критической температуры " << currentTemp << "C" << std::endl;
                    syslog(LOG_INFO, "Auto-optimization: Frequency reduced to minimum %d kHz due to critical temperature %.2fC", freqs.back(), currentTemp);
                }
            } else if (currentTemp > temp_monitor.warning_threshold) {
                if (currentGovernor != "ondemand") {
                    SetCpuGovernor("ondemand");
                    std::cout << "Автооптимизация: Переключение на governor ondemand из-за высокой температуры " << currentTemp << "C" << std::endl;
                    syslog(LOG_INFO, "Auto-optimization: Switched to ondemand governor due to high temperature %.2fC", currentTemp);
                }
                // Снижаем частоту на один шаг, если температура высокая
                std::vector<int> freqs = getAvailableCpuFrequencies();
                if (!freqs.empty()) {
                    for (size_t i = 0; i < freqs.size() - 1; ++i) {
                        if (currentFrequency == freqs[i]) {
                            SetCpuFrequency(freqs[i + 1]);
                            std::cout << "Автооптимизация: Частота снижена до " << freqs[i + 1] << " kHz из-за высокой температуры " << currentTemp << "C" << std::endl;
                            syslog(LOG_INFO, "Auto-optimization: Frequency reduced to %d kHz due to high temperature %.2fC", freqs[i + 1], currentTemp);
                            break;
                        }
                    }
                }
            } else if (predictor.enabled && predictor.predicted_load > 85.0f && currentGovernor != "performance") {
                SetCpuGovernor("performance");
                std::cout << "Автооптимизация: Переключение на governor performance из-за высокой прогнозируемой нагрузки " << predictor.predicted_load << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Switched to performance governor due to high predicted load %.2f%%", predictor.predicted_load);
                // Устанавливаем максимальную частоту для производительности
                std::vector<int> freqs = getAvailableCpuFrequencies();
                if (!freqs.empty() && currentFrequency != freqs.front()) {
                    SetCpuFrequency(freqs.front());
                    std::cout << "Автооптимизация: Частота увеличена до максимальной " << freqs.front() << " kHz из-за высокой прогнозируемой нагрузки " << predictor.predicted_load << "%" << std::endl;
                    syslog(LOG_INFO, "Auto-optimization: Frequency increased to maximum %d kHz due to high predicted load %.2f%%", freqs.front(), predictor.predicted_load);
                }
            } else if (predictor.enabled && predictor.predicted_load < 15.0f && currentGovernor != "powersave" && currentTemp < temp_monitor.warning_threshold - 10.0f) {
                SetCpuGovernor("powersave");
                std::cout << "Автооптимизация: Переключение на governor powersave из-за низкой прогнозируемой нагрузки " << predictor.predicted_load << "% и низкой температуры " << currentTemp << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Switched to powersave governor due to low predicted load %.2f%% and low temperature %.2fC", predictor.predicted_load, currentTemp);
            } else if (currentLoad >= 20.0f && currentLoad <= 75.0f && currentGovernor != "ondemand" && currentTemp < temp_monitor.warning_threshold - 5.0f) {
                SetCpuGovernor("ondemand");
                std::cout << "Автооптимизация: Переключение на governor ondemand из-за умеренной нагрузки " << currentLoad << "% и приемлемой температуры " << currentTemp << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Switched to ondemand governor due to moderate load %.2f%% and acceptable temperature %.2fC", currentLoad, currentTemp);
            }

            // Дополнительная защита от износа: снижение частоты при экстремальной температуре
            if (currentTemp > 95.0f && currentFrequency > 0) {
                std::vector<int> freqs = getAvailableCpuFrequencies();
                if (!freqs.empty() && currentFrequency != freqs.back()) {
                    SetCpuFrequency(freqs.back());
                    std::cout << "Автооптимизация: Частота снижена до минимальной " << freqs.back() << " kHz из-за экстремальной температуры " << currentTemp << "C" << std::endl;
                    syslog(LOG_INFO, "Auto-optimization: Frequency reduced to minimum %d kHz due to extreme temperature %.2fC", freqs.back(), currentTemp);
                }
            }

            // Дополнительное логирование для отслеживания изменений
            static std::string lastGovernor = currentGovernor;
            static int lastFrequency = currentFrequency;
            static float lastLoad = currentLoad;
            static float lastTemp = currentTemp;
            static float lastPredictedLoad = predictor.predicted_load;

            if (lastGovernor != currentGovernor) {
                std::cout << "Автооптимизация: Изменение governor с " << lastGovernor << " на " << currentGovernor << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Governor changed from %s to %s", lastGovernor.c_str(), currentGovernor.c_str());
                lastGovernor = currentGovernor;
            }
            if (lastFrequency != currentFrequency) {
                std::cout << "Автооптимизация: Изменение частоты с " << lastFrequency << " kHz на " << currentFrequency << " kHz" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Frequency changed from %d kHz to %d kHz", lastFrequency, currentFrequency);
                lastFrequency = currentFrequency;
            }
            if (std::abs(lastLoad - currentLoad) > 5.0f) {
                std::cout << "Автооптимизация: Изменение нагрузки с " << lastLoad << "% на " << currentLoad << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Load changed from %.2f%% to %.2f%%", lastLoad, currentLoad);
                lastLoad = currentLoad;
            }
            if (std::abs(lastPredictedLoad - predictor.predicted_load) > 5.0f && predictor.enabled) {
                std::cout << "Автооптимизация: Изменение прогнозируемой нагрузки с " << lastPredictedLoad << "% на " << predictor.predicted_load << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Predicted load changed from %.2f%% to %.2f%%", lastPredictedLoad, predictor.predicted_load);
                lastPredictedLoad = predictor.predicted_load;
            }
            if (std::abs(lastTemp - currentTemp) > 3.0f) {
                std::cout << "Автооптимизация: Изменение температуры с " << lastTemp << "C на " << currentTemp << "C" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Temperature changed from %.2fC to %.2fC", lastTemp, currentTemp);
                lastTemp = currentTemp;
            }

            std::this_thread::sleep_for(std::chrono::seconds(15)); // Проверка каждые 15 секунд для снижения нагрузки
        }
    }

public:
    CpuOptimizer() : isInitialized(false), currentGovernor("ondemand"), currentFrequency(0), autoOptimizationEnabled(false), currentLoad(0.0f), isRoot(false) {
        std::cout << "Конструктор CpuOptimizer вызван" << std::endl;
        openlog("OptiCoreCpuOptimizer", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "CpuOptimizer constructor called");
    }

    ~CpuOptimizer() {
        std::cout << "Деструктор CpuOptimizer вызван" << std::endl;
        autoOptimizationEnabled = false; // Остановить автооптимизацию
        syslog(LOG_INFO, "CpuOptimizer destructor called");
        closelog();
    }

    bool Initialize() override {
        std::cout << "Инициализация CpuOptimizer" << std::endl;
        isInitialized = true;
        isRoot = checkRoot();
        if (!isRoot) {
            std::cerr << "Предупреждение: Программа запущена без прав root, некоторые функции оптимизации CPU могут быть недоступны." << std::endl;
            syslog(LOG_WARNING, "Program running without root privileges, some CPU optimization features may be unavailable");
        }
        currentFrequency = getCurrentCpuFrequency();
        temp_monitor.current_temperature = getCpuTemperature();
        std::cout << "CpuOptimizer успешно инициализирован" << std::endl;
        syslog(LOG_INFO, "CpuOptimizer initialized successfully");
        return true;
    }

    bool SetCpuGovernor(const std::string& governor) override {
        std::cout << "Установка CPU governor на " << governor << std::endl;
        if (!isInitialized) {
            std::cerr << "CpuOptimizer не инициализирован при попытке установки governor" << std::endl;
            syslog(LOG_ERR, "CpuOptimizer not initialized when attempting to set governor");
            return false;
        }
        std::string path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
        if (writeToFile(path, governor)) {
            currentGovernor = governor;
            std::cout << "CPU governor установлен на " << governor << std::endl;
            syslog(LOG_INFO, "CPU governor set to %s", governor.c_str());
            return true;
        } else {
            std::cerr << "Ошибка при установке CPU governor" << std::endl;
            syslog(LOG_ERR, "Failed to set CPU governor");
            return false;
        }
    }

    bool EnableLoadPrediction(bool enable) override {
        std::cout << "Установка предсказания нагрузки CPU: " << (enable ? "включено" : "выключено") << std::endl;
        if (!isInitialized) {
            std::cerr << "CpuOptimizer не инициализирован при попытке установки предсказания нагрузки" << std::endl;
            syslog(LOG_ERR, "CpuOptimizer not initialized when attempting to set load prediction");
            return false;
        }
        predictor.enabled = enable;
        if (enable) {
            predictor.predicted_load = calculateCpuLoad();
        } else {
            predictor.predicted_load = 0.0f;
        }
        std::cout << "Предсказание нагрузки CPU " << (enable ? "включено" : "выключено") << ", текущая нагрузка: " << predictor.predicted_load << "%" << std::endl;
        syslog(LOG_INFO, "CPU load prediction %s, current load: %.2f%%", enable ? "enabled" : "disabled", predictor.predicted_load);
        return true;
    }

    bool EnableTemperatureMonitoring(bool enable, float warning_threshold = 70.0f, float critical_threshold = 85.0f) override {
        std::cout << "Установка мониторинга температуры CPU: " << (enable ? "включено" : "выключено") << " с порогами предупреждения=" << warning_threshold << "C и критическим=" << critical_threshold << "C" << std::endl;
        if (!isInitialized) {
            std::cerr << "CpuOptimizer не инициализирован при попытке установки мониторинга температуры" << std::endl;
            syslog(LOG_ERR, "CpuOptimizer not initialized when attempting to set temperature monitoring");
            return false;
        }
        if (warning_threshold < 0 || warning_threshold > critical_threshold || critical_threshold > 120) {
            std::cerr << "Ошибка: Неверные пороги температуры, warning должен быть меньше critical и в разумных пределах" << std::endl;
            syslog(LOG_ERR, "Invalid temperature thresholds, warning must be less than critical and within reasonable limits");
            return false;
        }
        temp_monitor.enabled = enable;
        temp_monitor.warning_threshold = warning_threshold;
        temp_monitor.critical_threshold = critical_threshold;
        if (enable) {
            temp_monitor.current_temperature = getCpuTemperature();
        } else {
            temp_monitor.current_temperature = 0.0f;
        }
        std::cout << "Мониторинг температуры CPU " << (enable ? "включен" : "выключен") << ", текущая температура: " << temp_monitor.current_temperature << "C" << std::endl;
        syslog(LOG_INFO, "CPU temperature monitoring %s, current temperature: %.2fC", enable ? "enabled" : "disabled", temp_monitor.current_temperature);
        return true;
    }

    bool SetCpuFrequency(int frequency) override {
        std::cout << "Установка частоты CPU на " << frequency << " kHz" << std::endl;
        if (!isInitialized) {
            std::cerr << "CpuOptimizer не инициализирован при попытке установки частоты" << std::endl;
            syslog(LOG_ERR, "CpuOptimizer not initialized when attempting to set frequency");
            return false;
        }
        if (frequency < 100000) { // Проверка на минимально разумную частоту (100 MHz)
            std::cerr << "Ошибка: Частота CPU слишком низкая, должна быть не менее 100000 kHz (100 MHz)" << std::endl;
            syslog(LOG_ERR, "CPU frequency too low, must be at least 100000 kHz (100 MHz)");
            return false;
        }
        // Проверка, входит ли частота в список доступных частот
        std::vector<int> freqs = getAvailableCpuFrequencies();
        if (!freqs.empty()) {
            bool validFrequency = false;
            for (int f : freqs) {
                if (f == frequency) {
                    validFrequency = true;
                    break;
                }
            }
            if (!validFrequency) {
                // Если точное совпадение не найдено, выбираем ближайшую доступную частоту
                int closestFreq = freqs[0];
                int minDiff = std::abs(frequency - closestFreq);
                for (int f : freqs) {
                    int diff = std::abs(frequency - f);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestFreq = f;
                    }
                }
                std::cerr << "Предупреждение: Частота " << frequency << " kHz не входит в список доступных частот. Используется ближайшая доступная частота " << closestFreq << " kHz." << std::endl;
                syslog(LOG_WARNING, "Frequency %d kHz is not in the list of available frequencies. Using closest available frequency %d kHz", frequency, closestFreq);
                frequency = closestFreq;
            }
        }
        std::string path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed";
        if (writeToFile(path, std::to_string(frequency))) {
            currentFrequency = frequency;
            std::cout << "Частота CPU установлена на " << frequency << " kHz" << std::endl;
            syslog(LOG_INFO, "CPU frequency set to %d kHz", frequency);
            return true;
        } else {
            std::cerr << "Ошибка при установке частоты CPU" << std::endl;
            syslog(LOG_ERR, "Failed to set CPU frequency");
            return false;
        }
    }

    bool EnableAutoOptimization(bool enable) override {
        std::cout << "Установка автооптимизации CPU: " << (enable ? "включено" : "выключено") << std::endl;
        if (!isInitialized) {
            std::cerr << "CpuOptimizer не инициализирован при попытке установки автооптимизации" << std::endl;
            syslog(LOG_ERR, "CpuOptimizer not initialized when attempting to set auto optimization");
            return false;
        }
        autoOptimizationEnabled = enable;
        if (enable) {
            // Включаем мониторинг температуры и предсказание нагрузки при автооптимизации
            if (!temp_monitor.enabled) {
                EnableTemperatureMonitoring(true);
            }
            if (!predictor.enabled) {
                EnableLoadPrediction(true);
            }
            std::thread(&CpuOptimizer::autoOptimize, this).detach();
        }
        std::cout << "Автооптимизация CPU " << (enable ? "включена" : "выключена") << std::endl;
        syslog(LOG_INFO, "CPU auto optimization %s", enable ? "enabled" : "disabled");
        return true;
    }

    std::string QueryStatus() override {
        std::cout << "Получение статуса CpuOptimizer" << std::endl;
        syslog(LOG_INFO, "Getting CpuOptimizer status");
        currentFrequency = getCurrentCpuFrequency();
        currentLoad = calculateCpuLoad();
        temp_monitor.current_temperature = getCpuTemperature();
        return "CpuOptimizer: Initialized=" + std::string(isInitialized ? "true" : "false") +
               ", RootPrivileges=" + std::string(isRoot ? "true" : "false") +
               ", Governor=" + currentGovernor +
               ", Frequency=" + std::to_string(currentFrequency) + "kHz" +
               ", CurrentLoad=" + std::to_string(currentLoad) + "%" +
               ", LoadPrediction=" + std::string(predictor.enabled ? "enabled" : "disabled") +
               ", PredictedLoad=" + std::to_string(predictor.predicted_load) + "%" +
               ", TemperatureMonitoring=" + std::string(temp_monitor.enabled ? "enabled" : "disabled") +
               ", CurrentTemperature=" + std::to_string(temp_monitor.current_temperature) + "C" +
               ", WarningThreshold=" + std::to_string(temp_monitor.warning_threshold) + "C" +
               ", CriticalThreshold=" + std::to_string(temp_monitor.critical_threshold) + "C" +
               ", AutoOptimization=" + std::string(autoOptimizationEnabled ? "enabled" : "disabled");
    }
};

// Экспортируемая функция для создания экземпляра
extern "C" ICpuOptimizer* CreateCpuOptimizer() {
    std::cout << "Создание экземпляра CpuOptimizer" << std::endl;
    syslog(LOG_INFO, "Creating CpuOptimizer instance");
    return new CpuOptimizer();
}
