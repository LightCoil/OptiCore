#pragma once
#include <string>

class IProfiler {
public:
    virtual ~IProfiler() = default;
    virtual void startSection(const std::string& name) = 0;
    virtual void endSection(const std::string& name) = 0;
    virtual void report() const = 0;
    virtual void reset() = 0;
}; 