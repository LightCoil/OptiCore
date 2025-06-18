#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include <cstdint>

// Структура для хранения статистических данных
struct GPUStats {
    float gpuTemperature;     // Температура GPU в градусах Цельсия
    float cpuTemperature;     // Температура CPU в градусах Цельсия
    float fanSpeedPercent;    // Скорость вентилятора в процентах от максимума
    uint32_t fps;             // Текущая частота кадров в секунду
    uint32_t commandCountBefore; // Количество команд до оптимизации
    uint32_t commandCountAfter;  // Количество команд после оптимизации
    float gpuLoadPercent;     // Загрузка GPU в процентах
};

// Инициализация модуля статистики
void InitStatisticsManager();

// Сбор текущих статистических данных с системы
void CollectStatistics();

// Получение текущей статистики
GPUStats GetCurrentStats();

// Обновление данных для HUD (Head-Up Display)
void UpdateStatisticsHUD();

// Деинициализация модуля статистики
void ShutdownStatisticsManager();

#endif // STATISTICS_MANAGER_H
