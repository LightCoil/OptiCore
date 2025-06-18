// include/system_utils.h
#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

// Поиск адреса GPU ring buffer
void* FindGPURingBuffer();

// Чтение текущего указателя на команды в ring buffer
void* ReadRingBuffer();

// Выделение локального буфера для копирования команд
void* AllocateLocalBuffer(void* commandBuffer);

// Установка хука на функцию GPU
void InstallHook(void* targetAddr, void* hookFunc);

// Удаление хука и восстановление оригинальной функции
void UninstallHook(void* hookFunc);

// Очистка указателей на ring buffer
void ClearRingBufferPointers();

// Отправка оптимизированных команд в GPU
void SubmitOptimizedCommands(void* optimizedCommands);

// Освобождение буфера команд
void FreeCommandBuffer(void* buffer);

// Проверка запроса на выход из программы
int IsExitRequested();

// Получение температуры GPU
float GetGPUTemperature();

// Расчет текущего FPS
float CalculateFPS();

// Получение процента загрузки GPU
float GetGPULoadPercentage();

// Задержка выполнения (в миллисекундах)
void Sleep(int milliseconds);

// Получение температуры CPU
float GetCPUTemperature();

// Получение скорости вентилятора в процентах
float GetFanSpeedPercent();

// Получение загрузки GPU в процентах
float GetGPULoadPercent();

// Получение количества команд до оптимизации
uint32_t GetCommandCountBeforeOptimization();

// Получение количества команд после оптимизации
uint32_t GetCommandCountAfterOptimization();

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_UTILS_H
