#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// HugePagesManager
typedef struct {
    size_t page_size;
    size_t allocated_pages;
    void** pages;
} hugepages_manager_t;

// Инициализация менеджера больших страниц
bool hugepages_manager_init(hugepages_manager_t* mgr, size_t page_size, size_t num_pages);
// Выделение большой страницы
void* hugepages_manager_alloc(hugepages_manager_t* mgr);
// Освобождение большой страницы
void hugepages_manager_free(hugepages_manager_t* mgr, void* page);
// Освобождение всех ресурсов
void hugepages_manager_destroy(hugepages_manager_t* mgr); 