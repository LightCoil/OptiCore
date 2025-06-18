#pragma once

// Статусы GPU
enum class GPUStatus {
    Idle,
    Busy,
    Overheated,
    Error,
    Optimizing,
    // ... другие статусы
}; 