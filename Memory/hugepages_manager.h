#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct hugepages_manager hugepages_manager_t;

bool hugepages_manager_init(hugepages_manager_t* mgr, size_t page_size, size_t num_pages);
void* hugepages_manager_alloc(hugepages_manager_t* mgr);
void hugepages_manager_free(hugepages_manager_t* mgr, void* page);
void hugepages_manager_destroy(hugepages_manager_t* mgr); 