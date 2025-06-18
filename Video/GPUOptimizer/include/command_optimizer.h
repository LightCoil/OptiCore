// include/command_optimizer.h
#ifndef COMMAND_OPTIMIZER_H
#define COMMAND_OPTIMIZER_H

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация оптимизатора команд
void InitCommandOptimizer();

// Оптимизация команд GPU
void* OptimizeCommands(void* commands);

// Получение количества команд до оптимизации
unsigned int GetCommandCountBefore();

// Получение количества команд после оптимизации
unsigned int GetCommandCountAfter();

// Деинициализация оптимизатора команд
void ShutdownCommandOptimizer();

#ifdef __cplusplus
}
#endif

#endif // COMMAND_OPTIMIZER_H
