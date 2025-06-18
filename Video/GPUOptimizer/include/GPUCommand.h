#pragma once
#include <cstdint>
#include <vector>

// Типы команд для GPU
enum class GPUCommandType {
    Draw,
    Compute,
    MemoryTransfer,
    Custom,
    // ... другие типы команд
};

// Структура команды GPU
struct GPUCommand {
    GPUCommandType type;
    std::vector<uint8_t> payload; // Сырые данные команды
    uint64_t timestamp;           // Метка времени
    // Метаданные, приоритет, идентификатор и т.д. можно добавить при необходимости
}; 