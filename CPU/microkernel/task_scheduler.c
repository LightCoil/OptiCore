/* task_scheduler.c - Планировщик задач для микроядра Xbox 360 */
/* Этот файл реализует минималистичный планировщик для распределения задач между ядрами */

#include <stdint.h>

#define MAX_TASKS 64

/* Структура для описания задачи */
typedef struct {
    void (*func)(void*);    /* Указатель на функцию задачи */
    void *arg;              /* Аргументы для функции */
    volatile int status;    /* Статус: 0 = новая, 1 = выполняется, 2 = завершена */
} task_t;

/* Очередь задач */
task_t task_queue[MAX_TASKS];
volatile int task_head = 0;  /* Индекс начала очереди */
volatile int task_tail = 0;  /* Индекс конца очереди */

/* Функция для добавления задачи в очередь (атомарная операция) */
int enqueue_task(void (*func)(void*), void *arg) {
    int old_tail;
    int new_tail;
    int reserved;
    
    do {
        old_tail = task_tail;
        if ((old_tail + 1) % MAX_TASKS == task_head) {
            return -1; // Очередь полна
        }
        new_tail = (old_tail + 1) % MAX_TASKS;
        
        // Используем атомарные операции для безопасного обновления tail
        // На PowerPC используем lwarx/stwcx. для реализации lock-free механизма
        asm volatile (
            "lwarx %0, 0, %2\n"
            "cmpw %0, %3\n"
            "bne- 1f\n"
            "stwcx. %4, 0, %2\n"
            "bne- 1f\n"
            "li %1, 1\n"
            "b 2f\n"
            "1: li %1, 0\n"
            "2:\n"
            : "=r"(old_tail), "=r"(reserved)
            : "r"(&task_tail), "r"(old_tail), "r"(new_tail)
            : "memory", "cr0"
        );
    } while (!reserved);
    
    task_queue[old_tail].func = func;
    task_queue[old_tail].arg = arg;
    task_queue[old_tail].status = 0;
    return 0;
}

/* Функция для получения следующей задачи из очереди (атомарная операция) */
task_t* get_next_task_atomic() {
    if (task_head == task_tail) {
        return 0; // Очередь пуста
    }
    
    // Используем атомарные операции для безопасного доступа к очереди
    // На PowerPC используем lwarx/stwcx. для реализации lock-free механизма
    uint32_t old_head;
    asm volatile (
        "lwarx %0, 0, %1\n"
        : "=r"(old_head)
        : "r"(&task_head)
        : "memory"
    );
    
    if (old_head == task_tail) {
        return 0; // Очередь пуста
    }
    
    task_t* task = &task_queue[old_head];
    task->status = 1; // Помечаем как выполняющуюся
    
    uint32_t new_head = (old_head + 1) % MAX_TASKS;
    uint32_t reserved;
    asm volatile (
        "stwcx. %2, 0, %1\n"
        "bne- 1f\n"
        "li %0, 1\n"
        "b 2f\n"
        "1: li %0, 0\n"
        "2:\n"
        : "=r"(reserved)
        : "r"(&task_head), "r"(new_head)
        : "memory", "cr0"
    );
    
    if (reserved) {
        return task;
    } else {
        return 0; // Не удалось обновить head, повторяем попытку
    }
}

/* Цикл рабочего ядра для обработки задач */
void worker_thread_loop(int id) {
    while (1) {
        task_t *task = get_next_task_atomic();
        if (!task) {
            continue; // Нет задач, продолжаем ожидание
        }
        task->func(task->arg); // Выполняем задачу
        task->status = 2; // Помечаем как завершённую
    }
}

/* Инициализация планировщика */
void init_scheduler() {
    task_head = 0;
    task_tail = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        task_queue[i].status = 2; // Все задачи изначально завершены
    }
}
