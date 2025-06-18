// include/hook_gpu_buffer.h
#ifndef HOOK_GPU_BUFFER_H
#define HOOK_GPU_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация перехвата GPU буфера
void InitHookGPUBuffer();

// Захват команд из GPU буфера
void* CaptureGPUCommands();

// Восстановление оригинальных хуков GPU
void RestoreGPUHooks();

#ifdef __cplusplus
}
#endif

#endif // HOOK_GPU_BUFFER_H
