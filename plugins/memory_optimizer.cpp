#include "memory_optimizer.h"
#include <iostream>
#include <string>
#include <syslog.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

struct swap_manager_t {
    bool enabled;
    int swappiness;
    int vfs_cache_pressure;

    swap_manager_t() : enabled(false), swappiness(60), vfs_cache_pressure(100) {
        std::cout << "Конструктор swap_manager_t вызван" << std::endl;
    }
};

struct prefetch_manager_t {
    bool enabled;
    int prefetch_level;
    bool readahead_enabled;
    int readahead_kb;

    prefetch_manager_t() : enabled(false), prefetch_level(3), readahead_enabled(false), readahead_kb(128) {
        std::cout << "Конструктор prefetch_manager_t вызван" << std::endl;
    }
};

struct hugepages_manager_t {
    bool enabled;
    int hugepages_count;
    bool transparent_hugepages_enabled;
    std::string thp_defrag;

    hugepages_manager_t() : enabled(false), hugepages_count(0), transparent_hugepages_enabled(false), thp_defrag("madvise") {
        std::cout << "Конструктор hugepages_manager_t вызван" << std::endl;
    }
};

class MemoryOptimizer : public IMemoryOptimizer {
private:
    bool isInitialized;
    swap_manager_t swap_mgr;
    prefetch_manager_t prefetch_mgr;
    hugepages_manager_t huge_mgr;
    bool autoOptimizationEnabled;
    float currentMemoryUsage;
    float currentSwapUsage;
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

    float getMemoryUsage() {
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/meminfo для получения использования памяти." << std::endl;
            syslog(LOG_ERR, "Failed to open /proc/meminfo for memory usage");
            return currentMemoryUsage; // Возвращаем предыдущее значение в случае ошибки
        }

        std::string line;
        long total = 0, free = 0, available = 0;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                sscanf(line.c_str(), "MemTotal: %ld kB", &total);
            } else if (line.find("MemFree:") != std::string::npos) {
                sscanf(line.c_str(), "MemFree: %ld kB", &free);
            } else if (line.find("MemAvailable:") != std::string::npos) {
                sscanf(line.c_str(), "MemAvailable: %ld kB", &available);
            }
        }

        if (total > 0) {
            return ((float)(total - available) / total) * 100.0f;
        }
        std::cerr << "Ошибка: Не удалось вычислить использование памяти, MemTotal равен 0." << std::endl;
        syslog(LOG_ERR, "Failed to calculate memory usage, MemTotal is 0");
        return currentMemoryUsage; // Возвращаем предыдущее значение в случае ошибки
    }

    float getSwapUsage() {
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/meminfo для получения использования swap." << std::endl;
            syslog(LOG_ERR, "Failed to open /proc/meminfo for swap usage");
            return currentSwapUsage; // Возвращаем предыдущее значение в случае ошибки
        }

        std::string line;
        long total = 0, free = 0;
        while (std::getline(meminfo, line)) {
            if (line.find("SwapTotal:") != std::string::npos) {
                sscanf(line.c_str(), "SwapTotal: %ld kB", &total);
            } else if (line.find("SwapFree:") != std::string::npos) {
                sscanf(line.c_str(), "SwapFree: %ld kB", &free);
            }
        }

        if (total > 0) {
            return ((float)(total - free) / total) * 100.0f;
        }
        std::cerr << "Ошибка: Не удалось вычислить использование swap, SwapTotal равен 0." << std::endl;
        syslog(LOG_ERR, "Failed to calculate swap usage, SwapTotal is 0");
        return currentSwapUsage; // Возвращаем предыдущее значение в случае ошибки
    }

    std::vector<std::string> getBlockDevices() {
        std::vector<std::string> devices;
        std::ifstream blockinfo("/proc/partitions");
        if (!blockinfo.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть /proc/partitions для получения списка устройств." << std::endl;
            syslog(LOG_ERR, "Failed to open /proc/partitions for block devices list");
            return devices;
        }

        std::string line;
        // Пропускаем заголовок
        std::getline(blockinfo, line);
        std::getline(blockinfo, line);
        while (std::getline(blockinfo, line)) {
            std::string device;
            std::istringstream iss(line);
            int major, minor, blocks;
            std::string name;
            iss >> major >> minor >> blocks >> name;
            if (!name.empty() && name.find("loop") == std::string::npos) {
                devices.push_back(name);
            }
        }
        return devices;
    }

    void autoOptimize() {
        while (autoOptimizationEnabled) {
            currentMemoryUsage = getMemoryUsage();
            currentSwapUsage = getSwapUsage();

            // Улучшенная автоматическая настройка swappiness
            if (currentMemoryUsage > 92.0f && swap_mgr.swappiness < 85) {
                SetSwappiness(85);
                std::cout << "Автооптимизация: Swappiness увеличен до 85 из-за критически высокого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Swappiness increased to 85 due to critical memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage < 45.0f && swap_mgr.swappiness > 35) {
                SetSwappiness(35);
                std::cout << "Автооптимизация: Swappiness снижен до 35 из-за низкого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Swappiness decreased to 35 due to low memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage >= 45.0f && currentMemoryUsage <= 92.0f && swap_mgr.swappiness != 60) {
                SetSwappiness(60);
                std::cout << "Автооптимизация: Swappiness установлен на 60 из-за умеренного использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Swappiness set to 60 due to moderate memory usage %.2f%%", currentMemoryUsage);
            }

            // Улучшенная автоматическая настройка vfs_cache_pressure
            if (currentMemoryUsage > 88.0f && swap_mgr.vfs_cache_pressure > 40) {
                SetVFSCachePressure(40);
                std::cout << "Автооптимизация: VFS cache pressure снижен до 40 из-за высокого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: VFS cache pressure decreased to 40 due to high memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage < 35.0f && swap_mgr.vfs_cache_pressure < 160) {
                SetVFSCachePressure(160);
                std::cout << "Автооптимизация: VFS cache pressure увеличен до 160 из-за низкого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: VFS cache pressure increased to 160 due to low memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage >= 35.0f && currentMemoryUsage <= 88.0f && swap_mgr.vfs_cache_pressure != 100) {
                SetVFSCachePressure(100);
                std::cout << "Автооптимизация: VFS cache pressure установлен на 100 из-за умеренного использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: VFS cache pressure set to 100 due to moderate memory usage %.2f%%", currentMemoryUsage);
            }

            // Автоматическая очистка кэшей при очень высоком использовании памяти
            if (currentMemoryUsage > 97.0f) {
                DropCaches();
                std::cout << "Автооптимизация: Кэши памяти очищены из-за критически высокого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Memory caches dropped due to critical memory usage %.2f%%", currentMemoryUsage);
            }

            // Улучшенная автоматическая настройка readahead для производительности ввода-вывода
            if (currentMemoryUsage < 25.0f && prefetch_mgr.readahead_kb < 768) {
                SetReadaheadKB(768);
                std::cout << "Автооптимизация: Readahead увеличен до 768 KB из-за очень низкого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Readahead increased to 768 KB due to very low memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage > 85.0f && prefetch_mgr.readahead_kb > 64) {
                SetReadaheadKB(64);
                std::cout << "Автооптимизация: Readahead снижен до 64 KB из-за высокого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Readahead decreased to 64 KB due to high memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage >= 25.0f && currentMemoryUsage <= 85.0f && prefetch_mgr.readahead_kb != 256) {
                SetReadaheadKB(256);
                std::cout << "Автооптимизация: Readahead установлен на 256 KB из-за умеренного использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Readahead set to 256 KB due to moderate memory usage %.2f%%", currentMemoryUsage);
            }

            // Улучшенная автоматическая настройка huge pages при использовании swap
            if (currentSwapUsage > 25.0f && !huge_mgr.enabled) {
                EnableHugePages(true);
                std::cout << "Автооптимизация: Huge pages включены из-за высокого использования swap " << currentSwapUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Huge pages enabled due to high swap usage %.2f%%", currentSwapUsage);
            } else if (currentSwapUsage < 3.0f && huge_mgr.enabled) {
                EnableHugePages(false);
                std::cout << "Автооптимизация: Huge pages отключены из-за очень низкого использования swap " << currentSwapUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Huge pages disabled due to very low swap usage %.2f%%", currentSwapUsage);
            }

            // Улучшенная настройка transparent huge pages
            if (currentMemoryUsage > 90.0f && !huge_mgr.transparent_hugepages_enabled) {
                EnableTransparentHugePages(true, "always");
                std::cout << "Автооптимизация: Transparent huge pages включены из-за критически высокого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Transparent huge pages enabled due to critical memory usage %.2f%%", currentMemoryUsage);
            } else if (currentMemoryUsage < 25.0f && huge_mgr.transparent_hugepages_enabled) {
                EnableTransparentHugePages(false, "never");
                std::cout << "Автооптимизация: Transparent huge pages отключены из-за низкого использования памяти " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Transparent huge pages disabled due to low memory usage %.2f%%", currentMemoryUsage);
            }

            // Дополнительная защита от износа: снижение swappiness при экстремальном использовании swap
            if (currentSwapUsage > 60.0f && swap_mgr.swappiness > 10) {
                SetSwappiness(10);
                std::cout << "Автооптимизация: Swappiness снижен до 10 из-за экстремального использования swap " << currentSwapUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Swappiness reduced to 10 due to extreme swap usage %.2f%%", currentSwapUsage);
            }

            // Дополнительное логирование для отслеживания изменений
            static float lastMemoryUsage = currentMemoryUsage;
            static float lastSwapUsage = currentSwapUsage;
            static int lastSwappiness = swap_mgr.swappiness;
            static int lastVFSCachePressure = swap_mgr.vfs_cache_pressure;
            static bool lastHugePagesEnabled = huge_mgr.enabled;
            static bool lastTransparentHugePagesEnabled = huge_mgr.transparent_hugepages_enabled;
            static int lastReadaheadKB = prefetch_mgr.readahead_kb;

            if (std::abs(lastMemoryUsage - currentMemoryUsage) > 5.0f) {
                std::cout << "Автооптимизация: Использование памяти изменилось с " << lastMemoryUsage << "% на " << currentMemoryUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Memory usage changed from %.2f%% to %.2f%%", lastMemoryUsage, currentMemoryUsage);
                lastMemoryUsage = currentMemoryUsage;
            }
            if (std::abs(lastSwapUsage - currentSwapUsage) > 5.0f) {
                std::cout << "Автооптимизация: Использование swap изменилось с " << lastSwapUsage << "% на " << currentSwapUsage << "%" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Swap usage changed from %.2f%% to %.2f%%", lastSwapUsage, currentSwapUsage);
                lastSwapUsage = currentSwapUsage;
            }
            if (lastSwappiness != swap_mgr.swappiness) {
                std::cout << "Автооптимизация: Swappiness изменился с " << lastSwappiness << " на " << swap_mgr.swappiness << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Swappiness changed from %d to %d", lastSwappiness, swap_mgr.swappiness);
                lastSwappiness = swap_mgr.swappiness;
            }
            if (lastVFSCachePressure != swap_mgr.vfs_cache_pressure) {
                std::cout << "Автооптимизация: VFS cache pressure изменился с " << lastVFSCachePressure << " на " << swap_mgr.vfs_cache_pressure << std::endl;
                syslog(LOG_INFO, "Auto-optimization: VFS cache pressure changed from %d to %d", lastVFSCachePressure, swap_mgr.vfs_cache_pressure);
                lastVFSCachePressure = swap_mgr.vfs_cache_pressure;
            }
            if (lastHugePagesEnabled != huge_mgr.enabled) {
                std::cout << "Автооптимизация: Huge pages изменился с " << (lastHugePagesEnabled ? "включено" : "выключено") << " на " << (huge_mgr.enabled ? "включено" : "выключено") << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Huge pages changed from %s to %s", lastHugePagesEnabled ? "enabled" : "disabled", huge_mgr.enabled ? "enabled" : "disabled");
                lastHugePagesEnabled = huge_mgr.enabled;
            }
            if (lastTransparentHugePagesEnabled != huge_mgr.transparent_hugepages_enabled) {
                std::cout << "Автооптимизация: Transparent huge pages изменился с " << (lastTransparentHugePagesEnabled ? "включено" : "выключено") << " на " << (huge_mgr.transparent_hugepages_enabled ? "включено" : "выключено") << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Transparent huge pages changed from %s to %s", lastTransparentHugePagesEnabled ? "enabled" : "disabled", huge_mgr.transparent_hugepages_enabled ? "enabled" : "disabled");
                lastTransparentHugePagesEnabled = huge_mgr.transparent_hugepages_enabled;
            }
            if (lastReadaheadKB != prefetch_mgr.readahead_kb) {
                std::cout << "Автооптимизация: Readahead изменился с " << lastReadaheadKB << " KB на " << prefetch_mgr.readahead_kb << " KB" << std::endl;
                syslog(LOG_INFO, "Auto-optimization: Readahead changed from %d KB to %d KB", lastReadaheadKB, prefetch_mgr.readahead_kb);
                lastReadaheadKB = prefetch_mgr.readahead_kb;
            }

            std::this_thread::sleep_for(std::chrono::seconds(20)); // Проверка каждые 20 секунд для снижения нагрузки
        }
    }

public:
    MemoryOptimizer() : isInitialized(false), autoOptimizationEnabled(false), currentMemoryUsage(0.0f), currentSwapUsage(0.0f), isRoot(false) {
        std::cout << "Конструктор MemoryOptimizer вызван" << std::endl;
        openlog("OptiCoreMemoryOptimizer", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "MemoryOptimizer constructor called");
    }

    ~MemoryOptimizer() {
        std::cout << "Деструктор MemoryOptimizer вызван" << std::endl;
        autoOptimizationEnabled = false; // Остановить автооптимизацию
        syslog(LOG_INFO, "MemoryOptimizer destructor called");
        closelog();
    }

    bool Initialize() override {
        std::cout << "Инициализация MemoryOptimizer" << std::endl;
        isInitialized = true;
        isRoot = checkRoot();
        if (!isRoot) {
            std::cerr << "Предупреждение: Программа запущена без прав root, некоторые функции оптимизации памяти могут быть недоступны." << std::endl;
            syslog(LOG_WARNING, "Program running without root privileges, some memory optimization features may be unavailable");
        }
        currentMemoryUsage = getMemoryUsage();
        currentSwapUsage = getSwapUsage();
        std::cout << "MemoryOptimizer успешно инициализирован" << std::endl;
        syslog(LOG_INFO, "MemoryOptimizer initialized successfully");
        return true;
    }

    bool SetSwappiness(int value) override {
        std::cout << "Установка swappiness на " << value << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки swappiness" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set swappiness");
            return false;
        }
        if (value < 0 || value > 100) {
            std::cerr << "Ошибка: Swappiness должен быть в диапазоне 0-100" << std::endl;
            syslog(LOG_ERR, "Swappiness must be in range 0-100");
            return false;
        }
        std::string path = "/proc/sys/vm/swappiness";
        if (writeToFile(path, std::to_string(value))) {
            swap_mgr.swappiness = value;
            swap_mgr.enabled = true;
            std::cout << "Swappiness установлен на " << value << std::endl;
            syslog(LOG_INFO, "Swappiness set to %d", value);
            return true;
        } else {
            std::cerr << "Ошибка при установке swappiness" << std::endl;
            syslog(LOG_ERR, "Failed to set swappiness");
            return false;
        }
    }

    bool SetVFSCachePressure(int value) override {
        std::cout << "Установка VFS cache pressure на " << value << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки VFS cache pressure" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set VFS cache pressure");
            return false;
        }
        if (value < 0 || value > 1000) {
            std::cerr << "Ошибка: VFS cache pressure должен быть в диапазоне 0-1000" << std::endl;
            syslog(LOG_ERR, "VFS cache pressure must be in range 0-1000");
            return false;
        }
        std::string path = "/proc/sys/vm/vfs_cache_pressure";
        if (writeToFile(path, std::to_string(value))) {
            swap_mgr.vfs_cache_pressure = value;
            std::cout << "VFS cache pressure установлен на " << value << std::endl;
            syslog(LOG_INFO, "VFS cache pressure set to %d", value);
            return true;
        } else {
            std::cerr << "Ошибка при установке VFS cache pressure" << std::endl;
            syslog(LOG_ERR, "Failed to set VFS cache pressure");
            return false;
        }
    }

    bool DropCaches() override {
        std::cout << "Очистка кэшей памяти" << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке очистки кэшей" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to drop caches");
            return false;
        }
        std::string path = "/proc/sys/vm/drop_caches";
        if (writeToFile(path, "3")) {
            std::cout << "Кэши памяти очищены" << std::endl;
            syslog(LOG_INFO, "Memory caches dropped");
            return true;
        } else {
            std::cerr << "Ошибка при очистке кэшей памяти" << std::endl;
            syslog(LOG_ERR, "Failed to drop memory caches");
            return false;
        }
    }

    bool EnableHugePages(bool enable) override {
        std::cout << "Установка использования huge pages: " << (enable ? "включено" : "выключено") << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки huge pages" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set huge pages");
            return false;
        }
        std::string path = "/proc/sys/vm/nr_hugepages";
        int count = enable ? 200 : 0; // Увеличенное значение для лучшей производительности
        if (writeToFile(path, std::to_string(count))) {
            huge_mgr.enabled = enable;
            huge_mgr.hugepages_count = count;
            std::cout << "Использование huge pages " << (enable ? "включено" : "выключено") << " с количеством страниц " << count << std::endl;
            syslog(LOG_INFO, "Huge pages %s with count %d", enable ? "enabled" : "disabled", count);
            return true;
        } else {
            std::cerr << "Ошибка при установке huge pages" << std::endl;
            syslog(LOG_ERR, "Failed to set huge pages");
            return false;
        }
    }

    bool EnableTransparentHugePages(bool enable, const std::string& defrag = "madvise") override {
        std::cout << "Установка использования transparent huge pages: " << (enable ? "включено" : "выключено") << " с defrag=" << defrag << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки transparent huge pages" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set transparent huge pages");
            return false;
        }
        std::string enabled_path = "/sys/kernel/mm/transparent_hugepage/enabled";
        std::string defrag_path = "/sys/kernel/mm/transparent_hugepage/defrag";
        std::string enabled_value = enable ? "always" : "never";
        if (writeToFile(enabled_path, enabled_value) && writeToFile(defrag_path, defrag)) {
            huge_mgr.transparent_hugepages_enabled = enable;
            huge_mgr.thp_defrag = defrag;
            std::cout << "Использование transparent huge pages " << (enable ? "включено" : "выключено") << " с defrag=" << defrag << std::endl;
            syslog(LOG_INFO, "Transparent huge pages %s with defrag=%s", enable ? "enabled" : "disabled", defrag.c_str());
            return true;
        } else {
            std::cerr << "Ошибка при установке transparent huge pages" << std::endl;
            syslog(LOG_ERR, "Failed to set transparent huge pages");
            return false;
        }
    }

    bool SetPrefetchLevel(int level) override {
        std::cout << "Установка уровня предвыборки на " << level << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки уровня предвыборки" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set prefetch level");
            return false;
        }
        if (level < 0 || level > 3) {
            std::cerr << "Ошибка: Уровень предвыборки должен быть в диапазоне 0-3" << std::endl;
            syslog(LOG_ERR, "Prefetch level must be in range 0-3");
            return false;
        }
        // Предполагается, что уровень предвыборки может быть установлен через параметр ядра или модуль
        // В данном случае мы не можем напрямую управлять prefetch level в Linux таким образом,
        // но мы можем использовать это как индикатор для других настроек
        prefetch_mgr.enabled = level > 0;
        prefetch_mgr.prefetch_level = level;
        std::cout << "Уровень предвыборки установлен на " << level << " (эмуляция, прямое управление недоступно)" << std::endl;
        syslog(LOG_INFO, "Prefetch level set to %d (emulation, direct control unavailable)", level);
        return true;
    }

    bool SetReadaheadKB(int kb) override {
        std::cout << "Установка readahead на " << kb << " KB" << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки readahead" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set readahead");
            return false;
        }
        if (kb < 32 || kb > 4096) {
            std::cerr << "Ошибка: Readahead должен быть в диапазоне 32-4096 KB" << std::endl;
            syslog(LOG_ERR, "Readahead must be in range 32-4096 KB");
            return false;
        }
        std::vector<std::string> devices = getBlockDevices();
        bool success = false;
        for (const auto& device : devices) {
            std::string path = "/sys/block/" + device + "/queue/read_ahead_kb";
            if (writeToFile(path, std::to_string(kb))) {
                std::cout << "Readahead установлен на " << kb << " KB для устройства " << device << std::endl;
                syslog(LOG_INFO, "Readahead set to %d KB for device %s", kb, device.c_str());
                success = true;
            } else {
                std::cerr << "Ошибка при установке readahead для устройства " << device << std::endl;
                syslog(LOG_ERR, "Failed to set readahead for device %s", device.c_str());
            }
        }
        if (success) {
            prefetch_mgr.readahead_enabled = true;
            prefetch_mgr.readahead_kb = kb;
        }
        return success;
    }

    bool EnableAutoOptimization(bool enable) override {
        std::cout << "Установка автооптимизации памяти: " << (enable ? "включено" : "выключено") << std::endl;
        if (!isInitialized) {
            std::cerr << "MemoryOptimizer не инициализирован при попытке установки автооптимизации" << std::endl;
            syslog(LOG_ERR, "MemoryOptimizer not initialized when attempting to set auto optimization");
            return false;
        }
        autoOptimizationEnabled = enable;
        if (enable) {
            std::thread(&MemoryOptimizer::autoOptimize, this).detach();
        }
        std::cout << "Автооптимизация памяти " << (enable ? "включена" : "выключена") << std::endl;
        syslog(LOG_INFO, "Memory auto optimization %s", enable ? "enabled" : "disabled");
        return true;
    }

    std::string QueryStatus() override {
        std::cout << "Получение статуса MemoryOptimizer" << std::endl;
        syslog(LOG_INFO, "Getting MemoryOptimizer status");
        currentMemoryUsage = getMemoryUsage();
        currentSwapUsage = getSwapUsage();
        return "MemoryOptimizer: Initialized=" + std::string(isInitialized ? "true" : "false") +
               ", RootPrivileges=" + std::string(isRoot ? "true" : "false") +
               ", MemoryUsage=" + std::to_string(currentMemoryUsage) + "%" +
               ", SwapUsage=" + std::to_string(currentSwapUsage) + "%" +
               ", Swappiness=" + std::to_string(swap_mgr.swappiness) +
               ", VFSCachePressure=" + std::to_string(swap_mgr.vfs_cache_pressure) +
               ", HugePages=" + std::string(huge_mgr.enabled ? "enabled" : "disabled") +
               ", HugePagesCount=" + std::to_string(huge_mgr.hugepages_count) +
               ", TransparentHugePages=" + std::string(huge_mgr.transparent_hugepages_enabled ? "enabled" : "disabled") +
               ", THPDefrag=" + huge_mgr.thp_defrag +
               ", PrefetchLevel=" + std::to_string(prefetch_mgr.prefetch_level) +
               ", ReadaheadKB=" + std::to_string(prefetch_mgr.readahead_kb) +
               ", AutoOptimization=" + std::string(autoOptimizationEnabled ? "enabled" : "disabled");
    }
};

// Экспортируемая функция для создания экземпляра
extern "C" IMemoryOptimizer* CreateMemoryOptimizer() {
    std::cout << "Создание экземпляра MemoryOptimizer" << std::endl;
    syslog(LOG_INFO, "Creating MemoryOptimizer instance");
    return new MemoryOptimizer();
}
