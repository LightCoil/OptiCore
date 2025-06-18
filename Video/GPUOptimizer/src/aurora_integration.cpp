// src/aurora_integration.cpp
#include "aurora_integration.h"
#include "system_utils.h"
#include "config_manager.h"

// Инициализация интеграции с Aurora
void InitAuroraIntegration() {
    // Инициализируем ресурсы для взаимодействия с Aurora
    InitializeAuroraAPI();
    // Регистрируем плагин в Aurora для отображения в UI
    RegisterPluginInAurora();
    // Устанавливаем начальный статус плагина
    UpdatePluginStatusInAurora("GPU Optimizer: Initialized");
}

// Регистрация плагина в Aurora для отображения в UI
int RegisterPluginInAurora() {
    // Регистрируем плагин с помощью API Aurora
    // Указываем имя плагина, версию и описание
    const char* pluginName = "GPU Optimizer X360";
    const char* pluginVersion = "1.0.0";
    const char* pluginDescription = "Optimizes GPU performance for Xbox 360";
    int result = AuroraRegisterPlugin(pluginName, pluginVersion, pluginDescription);
    if (result == 1) {
        // Успешная регистрация
        UpdatePluginStatusInAurora("GPU Optimizer: Registered in Aurora");
    } else {
        // Ошибка регистрации
        UpdatePluginStatusInAurora("GPU Optimizer: Registration Failed");
    }
    return result;
}

// Открытие окна настроек плагина через Aurora UI
void OpenPluginSettingsInAurora() {
    // Открываем окно настроек через API Aurora
    AuroraOpenPluginSettings("GPU Optimizer X360");
    // Обновляем статус
    UpdatePluginStatusInAurora("GPU Optimizer: Settings Opened");
}

// Обновление статуса плагина в Aurora UI
void UpdatePluginStatusInAurora(const char* status) {
    // Обновляем статус через API Aurora
    AuroraUpdatePluginStatus("GPU Optimizer X360", status);
}

// Получение команды от Aurora UI (например, включить/отключить оптимизации)
int GetCommandFromAuroraUI(char* commandBuffer, int bufferSize) {
    // Получаем команду через API Aurora
    return AuroraGetPluginCommand("GPU Optimizer X360", commandBuffer, bufferSize);
}

// Деинициализация интеграции с Aurora
void ShutdownAuroraIntegration() {
    // Обновляем статус перед завершением
    UpdatePluginStatusInAurora("GPU Optimizer: Shutting Down");
    // Отключаем плагин от Aurora
    AuroraUnregisterPlugin("GPU Optimizer X360");
    // Освобождаем ресурсы API Aurora
    ShutdownAuroraAPI();
}
