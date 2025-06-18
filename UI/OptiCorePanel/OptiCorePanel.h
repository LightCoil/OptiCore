#pragma once
#include <memory>
#include <vector>
#include <string>

class ICpuOptimizer;
class IMemoryOptimizer;
class IGPUOptimizer;
class IAudioOptimizer;
class IPowerManager;
class IIOOptimizer;

class OptiCorePanel {
public:
    OptiCorePanel(std::shared_ptr<ICpuOptimizer> cpu,
                  std::shared_ptr<IMemoryOptimizer> mem,
                  std::shared_ptr<IGPUOptimizer> gpu,
                  std::shared_ptr<IAudioOptimizer> audio,
                  std::shared_ptr<IPowerManager> power,
                  std::shared_ptr<IIOOptimizer> io);
    void Render(); // Основной метод отрисовки ImGui UI
    void Update(); // Обновление данных/метрик
    void ShowAnalyticsTab(); // вкладка аналитики
    void ShowProfilesTab();  // вкладка профилей
    void ShowScenariosTab(); // вкладка сценариев
    void ShowLogsTab();      // вкладка логов
private:
    std::shared_ptr<ICpuOptimizer> cpu_;
    std::shared_ptr<IMemoryOptimizer> mem_;
    std::shared_ptr<IGPUOptimizer> gpu_;
    std::shared_ptr<IAudioOptimizer> audio_;
    std::shared_ptr<IPowerManager> power_;
    std::shared_ptr<IIOOptimizer> io_;
    // Кэшированные метрики и статусы
    std::string cpu_status_, mem_status_, gpu_status_, audio_status_, power_status_, io_status_;
    float cpu_usage_[60] = {};
    float mem_usage_[60] = {};
    float gpu_usage_[60] = {};
    float power_usage_[60] = {};
    float io_ops_[60] = {};
    float audio_quality_[60] = {};
    // --- Аналитика ---
    struct AnalyticsEntry {
        std::string metric;
        float before;
        float after;
        float efficiency;
        std::string timestamp;
    };
    std::vector<AnalyticsEntry> analytics_history_;
    // --- Профили ---
    struct Profile {
        std::string name;
        std::string description;
        // параметры профиля (governor, swappiness и т.д.)
    };
    std::vector<Profile> profiles_;
    int active_profile_ = 0;
    // --- Сценарии ---
    struct Scenario {
        std::string name;
        std::string description;
        std::vector<std::string> actions;
    };
    std::vector<Scenario> scenarios_;
    // --- Логи ---
    std::vector<std::string> logs_;
    void AddLog(const std::string& msg);
    // ... другие поля для графиков, профилей, логов ...
}; 