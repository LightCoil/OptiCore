// include/aurora_integration.h
#ifndef AURORA_INTEGRATION_H
#define AURORA_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация интеграции с Aurora
void InitAuroraIntegration();

// Регистрация плагина в Aurora для отображения в UI
int RegisterPluginInAurora();

// Открытие окна настроек плагина через Aurora UI
void OpenPluginSettingsInAurora();

// Обновление статуса плагина в Aurora UI
void UpdatePluginStatusInAurora(const char* status);

// Получение команды от Aurora UI (например, включить/отключить оптимизации)
int GetCommandFromAuroraUI(char* commandBuffer, int bufferSize);

// Деинициализация интеграции с Aurora
void ShutdownAuroraIntegration();

#ifdef __cplusplus
}
#endif

#endif // AURORA_INTEGRATION_H
