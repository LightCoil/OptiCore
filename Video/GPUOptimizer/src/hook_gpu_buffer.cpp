// src/hook_gpu_buffer.cpp
#include "hook_gpu_buffer.h"
#include "system_utils.h"

// Инициализация перехвата GPU буфера
void InitHookGPUBuffer() {
    // Находим адрес GPU ring buffer через системные вызовы Xbox 360
    void* ringBufferAddr = FindGPURingBuffer();
    if (ringBufferAddr) {
        // Устанавливаем хук на функцию отправки команд в GPU
        InstallHook(ringBufferAddr, &CustomGPUSubmitHandler);
        // Инициализируем внутренние структуры для отслеживания состояния буфера
        InitializeBufferTracking();
    }
}

// Захват команд из GPU буфера
void* CaptureGPUCommands() {
    // Читаем текущий указатель на команды в ring buffer
    void* commandBuffer = ReadRingBuffer();
    if (commandBuffer) {
        // Проверяем целостность данных в буфере
        if (ValidateBufferIntegrity(commandBuffer)) {
            // Копируем команды в локальный буфер для анализа
            void* localBuffer = AllocateLocalBuffer(commandBuffer);
            // Обновляем статистику захвата команд
            UpdateCaptureStats(localBuffer);
            return localBuffer;
        }
    }
    return nullptr;
}

// Восстановление оригинальных хуков GPU
void RestoreGPUHooks() {
    // Удаляем установленные хуки и восстанавливаем оригинальную функцию
    UninstallHook(&CustomGPUSubmitHandler);
    // Очищаем указатели на ring buffer
    ClearRingBufferPointers();
    // Сбрасываем состояние отслеживания буфера
    ResetBufferTracking();
}
