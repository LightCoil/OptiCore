// include/optimization_utils.h
#ifndef OPTIMIZATION_UTILS_H
#define OPTIMIZATION_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

// Загрузка правил оптимизации из конфигурации
void LoadOptimizationRulesFromConfig();

// Инициализация буферов анализа команд
void InitializeCommandAnalysisBuffers();

// Настройка кэша состояний для оптимизации
void SetupStateCache();

// Устранение избыточного overdraw
void EliminateOverdraw(void* commands);

// Объединение схожих draw calls
void MergeDrawCalls(void* commands);

// Упрощение изменений состояний шейдеров
void SimplifyShaderStateChanges(void* commands);

// Применение ранней отсечки Z-buffer
void ApplyEarlyZCulling(void* commands);

// Оптимизация порядка команд для параллельности
void ReorderCommandsForParallelism(void* commands);

// Освобождение буферов анализа команд
void FreeCommandAnalysisBuffers();

// Очистка правил оптимизации
void ClearOptimizationRules();

// Очистка кэша состояний
void ClearStateCache();

// Получение размера буфера команд
unsigned int GetCommandBufferSize(void* commandBuffer);

// Проверка целостности буфера команд
int ValidateBufferIntegrity(void* commandBuffer);

// Обновление статистики захвата команд
void UpdateCaptureStats(void* localBuffer);

#ifdef __cplusplus
}
#endif

#endif // OPTIMIZATION_UTILS_H
