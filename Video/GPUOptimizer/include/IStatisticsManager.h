#pragma once

class IStatisticsManager {
public:
    virtual ~IStatisticsManager() = default;
    virtual void collectStats() = 0;
    virtual void reportStats() = 0;
    virtual void resetStats() = 0;
}; 