#include <iostream>
#include "../include/IMemoryExtender.h"

class MemoryExtender : public IMemoryExtender {
public:
    MemoryExtender() = default;
    ~MemoryExtender() override = default;

    bool extendMemory(size_t size) override {
        std::cout << "MemoryExtender: расширение видеопамяти на " << size << " байт (HDD/SSD)" << std::endl;
        return true;
    }
    bool swapIn(void* dst, size_t size) override {
        std::cout << "MemoryExtender: swapIn " << size << " байт из HDD/SSD" << std::endl;
        return true;
    }
    bool swapOut(void* src, size_t size) override {
        std::cout << "MemoryExtender: swapOut " << size << " байт в HDD/SSD" << std::endl;
        return true;
    }
    void cacheManagement() override {
        std::cout << "MemoryExtender: управление кэшем видеопамяти" << std::endl;
    }
}; 