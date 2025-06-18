/* powerpc_utils.c - Утилиты для низкоуровневого доступа к PowerPC (Xenon) на Xbox 360 */
/* Этот файл содержит функции для работы с регистрами и инструкциями PowerPC */

#include <stdint.h>

/* Функция для чтения значения из специального регистра (SPR) */
uint32_t read_spr(uint32_t spr_num) {
    uint32_t value;
    asm volatile ("mfspr %0, %1" : "=r"(value) : "i"(spr_num));
    return value;
}

/* Функция для записи значения в специальный регистр (SPR) */
void write_spr(uint32_t spr_num, uint32_t value) {
    asm volatile ("mtspr %0, %1" : : "i"(spr_num), "r"(value) : "memory");
}

/* Функция для чтения Machine State Register (MSR) */
uint32_t read_msr() {
    uint32_t value;
    asm volatile ("mfmsr %0" : "=r"(value));
    return value;
}

/* Функция для записи в Machine State Register (MSR) */
void write_msr(uint32_t value) {
    asm volatile ("mtmsr %0" : : "r"(value) : "memory");
}

/* Функция для отправки Inter-Processor Interrupt (IPI) */
void send_ipi_to_core(uint32_t core_id, uint32_t vector) {
    // Используется доступ к регистрам для отправки IPI
    // Пример: запись в регистр IPI для указанного ядра (предполагаемый регистр SPR 1023 для Xenon)
    uint32_t ipi_value = (1 << core_id) | (vector & 0xFF);
    write_spr(1023, ipi_value); // SPR 1023 - предполагаемый регистр для IPI
    cache_coherency_sync(); // Обеспечение когерентности после записи
}

/* Функция для чтения Time Base Register (TBR) */
uint64_t read_timebase() {
    uint32_t tbl, tbu;
    asm volatile (
        "mftb %0\n"
        "mftbu %1\n"
        : "=r"(tbl), "=r"(tbu)
    );
    return ((uint64_t)tbu << 32) | tbl;
}

/* Функция для сброса TLB (Translation Lookaside Buffer) */
void invalidate_tlb() {
    asm volatile ("tlbia" : : : "memory");
}

/* Функция для записи записи в TLB */
void write_tlb_entry(uint32_t index, uint32_t virtual_addr, uint32_t physical_addr, uint32_t attributes) {
    // Подготовка значений для записи в TLB
    // Используется инструкция tlbwe (пример для PowerPC)
    // Формат записи зависит от архитектуры Xenon, здесь приведен упрощенный пример
    uint32_t tlb_hi = (virtual_addr & 0xFFFFF000) | (attributes & 0xFFF);
    uint32_t tlb_lo = (physical_addr & 0xFFFFF000) | (attributes & 0xFFF);
    
    // Запись в специальные регистры TLB (предполагаемые номера для Xenon)
    write_spr(688, tlb_hi); // SPR 688 - TLB High
    write_spr(689, tlb_lo); // SPR 689 - TLB Low
    write_spr(690, index);  // SPR 690 - TLB Index
    
    // Выполнение инструкции tlbwe для записи в TLB
    asm volatile ("tlbwe" : : : "memory");
}

/* Функция для обеспечения когерентности кэша */
void cache_coherency_sync() {
    asm volatile (
        "sync\n"
        "isync\n"
        : : : "memory"
    );
}

/* Функция для очистки кэша данных */
void flush_data_cache(void* addr, uint32_t size) {
    // Очистка кэша данных для указанного диапазона адресов
    // Используется инструкция dcbst для сброса кэша в память
    // Пример для PowerPC, адаптированный для Xenon
    uint8_t* ptr = (uint8_t*)addr;
    uint8_t* end = ptr + size;
    const uint32_t cache_line_size = 32; // Предполагаемый размер строки кэша для Xenon
    
    while (ptr < end) {
        asm volatile ("dcbst 0, %0" : : "r"(ptr) : "memory");
        ptr += cache_line_size;
    }
    cache_coherency_sync(); // Обеспечение когерентности после очистки
}

/* Инициализация низкоуровневых утилит */
void init_powerpc_utils() {
    // Здесь можно добавить дополнительную инициализацию
    // Например, настройка начальных значений регистров или проверка доступности инструкций
}
