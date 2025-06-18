#pragma once

class IResourceMonitor {
public:
    virtual ~IResourceMonitor() = default;
    virtual void monitorTemperature() = 0;
    virtual void monitorMemory() = 0;
    virtual void monitorDisk() = 0;
    virtual void alertIfOverload() = 0;
}; 