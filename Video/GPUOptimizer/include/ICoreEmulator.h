#pragma once
#include "GPUCommand.h"
#include <string>

class ICoreEmulator {
public:
    virtual ~ICoreEmulator() = default;
    virtual void emulateCommand(const GPUCommand&) = 0;
    virtual bool loadMicrocode(const std::string&) = 0;
}; 