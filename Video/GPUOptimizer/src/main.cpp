#include "../include/GPUManagerImpl.h"
#include <iostream>

int main() {
    std::cout << "=== Тест GPUManagerImpl ===" << std::endl;
    GPUManagerImpl manager;
    manager.initialize();

    GPUCommand drawCmd{GPUCommandType::Draw, {0x01, 0x02, 0x03}, 1001};
    GPUCommand computeCmd{GPUCommandType::Compute, {0x10, 0x20}, 1002};
    GPUCommand memCmd{GPUCommandType::MemoryTransfer, {0xFF, 0xEE}, 1003};
    GPUCommand customCmd{GPUCommandType::Custom, {0xAA, 0xBB}, 1004};

    manager.routeCommand(drawCmd);
    manager.routeCommand(computeCmd);
    manager.routeCommand(memCmd);
    manager.routeCommand(customCmd);

    manager.shutdown();
    std::cout << "=== Завершено ===" << std::endl;
    return 0;
} 