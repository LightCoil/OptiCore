#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Prefetch-стратегия
typedef enum {
    PREFETCH_LOOKAHEAD,
    PREFETCH_MARKOV
} prefetch_strategy_t;

// PrefetchManager
typedef struct {
    prefetch_strategy_t strategy;
    size_t lookahead_depth;
    // Марковская матрица вероятностей
    float* markov_matrix;
    size_t markov_size;
    // Callback для загрузки блока
    void (*load_block_cb)(uint64_t block_id);
} prefetch_manager_t;

// Инициализация PrefetchManager
void prefetch_manager_init(prefetch_manager_t* mgr, prefetch_strategy_t strategy, size_t lookahead_depth);
// Предиктивная подгрузка
void prefetch_manager_prefetch(prefetch_manager_t* mgr, uint64_t current_block_id);
// Интеграция с swap_manager
void prefetch_manager_set_load_cb(prefetch_manager_t* mgr, void (*load_block_cb)(uint64_t));
// Освобождение ресурсов
void prefetch_manager_destroy(prefetch_manager_t* mgr); 