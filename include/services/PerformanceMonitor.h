#pragma once

#include <Arduino.h>

struct PerformanceSnapshot {
    uint32_t lastSweepUs = 0;
    uint32_t averageSweepUs = 0;
    uint32_t maxSweepUs = 0;
    uint32_t averageUiUs = 0;
    uint32_t maxUiUs = 0;
    uint16_t loopsPerSecond = 0;
};

class PerformanceMonitor {
public:
    void tickLoop();
    void recordSweep(uint32_t durationUs);
    void recordUi(uint32_t durationUs);
    PerformanceSnapshot snapshot() const;

private:
    uint32_t lastSweepUs = 0;
    uint32_t averageSweepUs = 0;
    uint32_t maxSweepUs = 0;
    uint32_t averageUiUs = 0;
    uint32_t maxUiUs = 0;
    uint32_t loopWindowStartedMs = 0;
    uint32_t loopWindowCount = 0;
    uint16_t loopsPerSecond = 0;
};

extern PerformanceMonitor performanceMonitor;
