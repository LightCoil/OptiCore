#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum prefetch_strategy prefetch_strategy_t;
typedef struct prefetch_manager prefetch_manager_t;

void prefetch_manager_init(prefetch_manager_t* mgr, prefetch_strategy_t strategy, size_t lookahead_depth);
void prefetch_manager_prefetch(prefetch_manager_t* mgr, uint64_t current_block_id);
void prefetch_manager_set_load_cb(prefetch_manager_t* mgr, void (*load_block_cb)(uint64_t));
void prefetch_manager_destroy(prefetch_manager_t* mgr); 