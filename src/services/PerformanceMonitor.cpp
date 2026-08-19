#include "services/PerformanceMonitor.h"

PerformanceMonitor performanceMonitor;

namespace {
uint32_t smooth(uint32_t previous, uint32_t sample) {
    return previous == 0 ? sample : (previous * 7U + sample) / 8U;
}
}

void PerformanceMonitor::tickLoop() {
    const uint32_t now = millis();
    if (loopWindowStartedMs == 0) loopWindowStartedMs = now;
    loopWindowCount++;
    const uint32_t elapsed = now - loopWindowStartedMs;
    if (elapsed >= 1000) {
        loopsPerSecond = static_cast<uint16_t>((loopWindowCount * 1000UL) / elapsed);
        loopWindowCount = 0;
        loopWindowStartedMs = now;
    }
}

void PerformanceMonitor::recordSweep(uint32_t durationUs) {
    lastSweepUs = durationUs;
    averageSweepUs = smooth(averageSweepUs, durationUs);
    if (durationUs > maxSweepUs) maxSweepUs = durationUs;
}

void PerformanceMonitor::recordUi(uint32_t durationUs) {
    averageUiUs = smooth(averageUiUs, durationUs);
    if (durationUs > maxUiUs) maxUiUs = durationUs;
}

PerformanceSnapshot PerformanceMonitor::snapshot() const {
    PerformanceSnapshot result;
    result.lastSweepUs = lastSweepUs;
    result.averageSweepUs = averageSweepUs;
    result.maxSweepUs = maxSweepUs;
    result.averageUiUs = averageUiUs;
    result.maxUiUs = maxUiUs;
    result.loopsPerSecond = loopsPerSecond;
    return result;
}
