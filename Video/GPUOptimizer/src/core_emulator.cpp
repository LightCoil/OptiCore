#include <iostream>
#include "../include/ICoreEmulator.h"

class CoreEmulator : public ICoreEmulator {
public:
    CoreEmulator() = default;
    ~CoreEmulator() override = default;

    void emulateCommand(const GPUCommand& cmd) override {
        std::cout << "CoreEmulator: эмуляция команды типа " << static_cast<int>(cmd.type) << std::endl;
    }
    bool loadMicrocode(const std::string& path) override {
        std::cout << "CoreEmulator: загрузка микрокода из " << path << std::endl;
        return true;
    }
}; 