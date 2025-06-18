// include/config_manager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация менеджера конфигурации
void InitConfigManager();

// Загрузка конфигурации из файла или системного хранилища
int LoadConfig(const char* configPath);

// Сохранение текущей конфигурации в файл или системное хранилище
int SaveConfig(const char* configPath);

// Получение текущего профиля оптимизации
const char* GetCurrentProfile();

// Установка профиля оптимизации по имени
int SetProfile(const char* profileName);

// Получение списка доступных профилей оптимизации
void GetAvailableProfiles(char** profiles, int* count);

// Установка значения параметра оптимизации для текущего профиля
int SetOptimizationParameter(const char* paramName, float value);

// Получение значения параметра оптимизации из текущего профиля
float GetOptimizationParameter(const char* paramName);

// Сброс конфигурации к значениям по умолчанию
void ResetConfigToDefault();

// Получение частоты обновления HUD (в миллисекундах)
int GetHUDUpdateFrequency();

// Установка частоты обновления HUD (в миллисекундах)
void SetHUDUpdateFrequency(int frequencyMs);

// Включение или отключение оптимизаций GPU
void EnableOptimizations(int enable);

// Проверка, включены ли оптимизации GPU
int AreOptimizationsEnabled();

// Освобождение ресурсов менеджера конфигурации
void ShutdownConfigManager();

#ifdef __cplusplus
}
#endif

#endif // CONFIG_MANAGER_H
