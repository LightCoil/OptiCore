#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct swap_block swap_block_t;
typedef struct swap_lru_node swap_lru_node_t;
typedef struct swap_manager swap_manager_t;

bool swap_manager_init(swap_manager_t* mgr, size_t max_blocks, const char* swap_file_path);
bool swap_manager_load_block(swap_manager_t* mgr, uint64_t block_id, void** out_data);
bool swap_manager_writeback_block(swap_manager_t* mgr, uint64_t block_id);
void swap_manager_evict_lru(swap_manager_t* mgr);
void swap_manager_destroy(swap_manager_t* mgr);
void swap_manager_prefetch(swap_manager_t* mgr, uint64_t* block_ids, size_t count); 