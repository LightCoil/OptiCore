#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Swap-блок
typedef struct {
    uint64_t block_id;
    void* data;
    bool dirty;
    bool in_use;
} swap_block_t;

// LRU-элемент
typedef struct swap_lru_node {
    swap_block_t* block;
    struct swap_lru_node* prev;
    struct swap_lru_node* next;
} swap_lru_node_t;

// Swap-менеджер
typedef struct {
    swap_lru_node_t* lru_head;
    swap_lru_node_t* lru_tail;
    size_t max_blocks;
    size_t used_blocks;
    // Платформенные дескрипторы
    int swap_fd; // Linux: файл swap, Windows: дескриптор файла
} swap_manager_t;

// Инициализация swap-менеджера
bool swap_manager_init(swap_manager_t* mgr, size_t max_blocks, const char* swap_file_path);
// Загрузка блока из swap
bool swap_manager_load_block(swap_manager_t* mgr, uint64_t block_id, void** out_data);
// Сброс блока в swap (writeback)
bool swap_manager_writeback_block(swap_manager_t* mgr, uint64_t block_id);
// LRU-эвикция
void swap_manager_evict_lru(swap_manager_t* mgr);
// Освобождение ресурсов
void swap_manager_destroy(swap_manager_t* mgr);
// Интеграция с PrefetchManager
void swap_manager_prefetch(swap_manager_t* mgr, uint64_t* block_ids, size_t count); 