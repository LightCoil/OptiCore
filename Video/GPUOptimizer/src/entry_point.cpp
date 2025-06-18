// src/entry_point.cpp
#include "hook_gpu_buffer.h"
#include "command_optimizer.h"
#include "hud_overlay.h"
#include "system_utils.h"
#include "statistics_manager.h"

extern "C" int __declspec(dllexport) main() {
// 1. Инициализация
    InitHookGPUBuffer();         // подключаемся к GPU ring buffer
    InitCommandOptimizer();      // загружаем правила оптимизации
    InitHUDOverlay();            // инициализируем HUD
    InitStatisticsManager();     // инициализируем модуль статистики
    InitAuroraIntegration();     // инициализируем интеграцию с Aurora

// 2. Основной цикл
    static int lastHUDUpdateTime = 0;
    while (!IsExitRequested()) {
        // Захватываем команды GPU для анализа
        void* commands = CaptureGPUCommands();
        if (commands) {
            // Проверяем, включены ли оптимизации
            if (AreOptimizationsEnabled()) {
                // Оптимизируем команды для повышения производительности
                void* optimized = OptimizeCommands(commands);
                // Отправляем оптимизированные команды обратно в GPU
                SubmitOptimizedCommands(optimized);
            } else {
                // Если оптимизации отключены, отправляем команды без изменений
                SubmitOptimizedCommands(commands);
            }
            // Освобождаем временные буферы
            FreeCommandBuffer(commands);
        }
        // Сбор статистики
        CollectStatistics();
        // Проверяем, пора ли обновить HUD согласно частоте из конфигурации
        int currentTime = XamGetSystemTime();
        int hudFrequency = GetHUDUpdateFrequency();
        if (currentTime - lastHUDUpdateTime >= hudFrequency) {
            // Обновляем и отрисовываем HUD с метриками
            UpdateStatisticsHUD();
            lastHUDUpdateTime = currentTime;
        }
        // Проверяем команды от Aurora UI
        char auroraCommand[128];
        if (GetCommandFromAuroraUI(auroraCommand, sizeof(auroraCommand)) > 0) {
            // Обрабатываем команды от Aurora (например, включить/отключить оптимизации)
            if (strcmp(auroraCommand, "enable_optimizations") == 0) {
                SetOptimizationsEnabled(1);
                UpdatePluginStatusInAurora("GPU Optimizer: Optimizations Enabled");
            } else if (strcmp(auroraCommand, "disable_optimizations") == 0) {
                SetOptimizationsEnabled(0);
                UpdatePluginStatusInAurora("GPU Optimizer: Optimizations Disabled");
            } else if (strcmp(auroraCommand, "open_settings") == 0) {
                OpenPluginSettingsInAurora();
            }
        }
        // Ограничиваем частоту цикла до ~60 FPS
        Sleep(16);
    }

// 3. Очистка
    ShutdownHUD();
    ShutdownAuroraIntegration();
    RestoreGPUHooks();
    return 0;
}
