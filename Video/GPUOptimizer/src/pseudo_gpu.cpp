#include <iostream>
#include "../include/IPseudoGPU.h"

class PseudoGPU : public IPseudoGPU {
public:
    PseudoGPU() = default;
    ~PseudoGPU() override = default;

    void processCommand(const GPUCommand& cmd) override {
        std::cout << "PseudoGPU: обработка команды типа " << static_cast<int>(cmd.type) << std::endl;
    }
    void* allocateVirtualMemory(size_t size) override {
        std::cout << "PseudoGPU: выделение виртуальной памяти, байт: " << size << std::endl;
        return new(std::nothrow) uint8_t[size];
    }
    void releaseVirtualMemory(void* ptr) override {
        std::cout << "PseudoGPU: освобождение виртуальной памяти" << std::endl;
        delete[] static_cast<uint8_t*>(ptr);
    }
}; 