#include "statistics_manager.h"
#include "hud_overlay.h"
#include "system_utils.h"
#include "../include/IStatisticsManager.h"
#include <iostream>

// Текущие статистические данные
static GPUStats currentStats = {0};

// Инициализация модуля статистики
void InitStatisticsManager() {
    // Сброс текущих значений
    currentStats.gpuTemperature = 0.0f;
    currentStats.cpuTemperature = 0.0f;
    currentStats.fanSpeedPercent = 0.0f;
    currentStats.fps = 0;
    currentStats.commandCountBefore = 0;
    currentStats.commandCountAfter = 0;
    currentStats.gpuLoadPercent = 0.0f;
    
    // Инициализация системных ресурсов для сбора данных
    // Здесь может быть инициализация доступа к сенсорам температуры и другим системным данным
}

// Сбор текущих статистических данных с системы
void CollectStatistics() {
    // Получение данных о температуре GPU и CPU
    currentStats.gpuTemperature = GetGPUTemperature(); // Функция из system_utils
    currentStats.cpuTemperature = GetCPUTemperature(); // Функция из system_utils
    
    // Получение скорости вентилятора
    currentStats.fanSpeedPercent = GetFanSpeedPercent(); // Функция из system_utils
    
    // Получение текущего FPS
    currentStats.fps = CalculateFPS(); // Функция из system_utils
    
    // Получение данных о командах GPU (до и после оптимизации)
    // Эти значения могут обновляться из модуля command_optimizer
    currentStats.commandCountBefore = GetCommandCountBeforeOptimization();
    currentStats.commandCountAfter = GetCommandCountAfterOptimization();
    
    // Получение загрузки GPU
    currentStats.gpuLoadPercent = GetGPULoadPercent(); // Функция из system_utils
}

// Получение текущей статистики
GPUStats GetCurrentStats() {
    return currentStats;
}

// Обновление данных для HUD (Head-Up Display)
void UpdateStatisticsHUD() {
    // Форматирование данных для отображения
    char hudText[256];
    snprintf(hudText, sizeof(hudText), 
             "GPU: %.1f°C | CPU: %.1f°C | Fan: %.1f%% | FPS: %u | Cmds: %u/%u | Load: %.1f%%",
             currentStats.gpuTemperature,
             currentStats.cpuTemperature,
             currentStats.fanSpeedPercent,
             currentStats.fps,
             currentStats.commandCountBefore,
             currentStats.commandCountAfter,
             currentStats.gpuLoadPercent);
    
    // Обновление HUD только при изменении данных
    UpdateHUDIfChanged(hudText); // Функция из hud_overlay
}

// Деинициализация модуля статистики
void ShutdownStatisticsManager() {
    // Очистка ресурсов, если они были выделены
    // Здесь может быть освобождение ресурсов, связанных с доступом к системным данным
}

class StatisticsManager : public IStatisticsManager {
public:
    StatisticsManager() = default;
    ~StatisticsManager() override = default;

    void collectStats() override {
        std::cout << "StatisticsManager: сбор статистики" << std::endl;
    }
    void reportStats() override {
        std::cout << "StatisticsManager: отчёт по статистике" << std::endl;
    }
    void resetStats() override {
        std::cout << "StatisticsManager: сброс статистики" << std::endl;
    }
};
