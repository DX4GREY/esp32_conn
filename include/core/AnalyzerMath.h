#pragma once

#include <stdint.h>

namespace AnalyzerMath {

inline uint8_t clampPercent(int value) {
    return value < 0 ? 0 : (value > 100 ? 100 : static_cast<uint8_t>(value));
}

inline uint8_t ema(uint8_t previous, uint8_t sample, uint8_t oldWeight = 3) {
    return static_cast<uint8_t>((static_cast<uint16_t>(previous) * oldWeight +
                                 sample) / (oldWeight + 1));
}

inline uint8_t deltaAboveBaseline(uint8_t sample, uint8_t baseline) {
    return sample > baseline ? static_cast<uint8_t>(sample - baseline) : 0;
}

inline uint8_t observationConfidence(int samplesPerChannel, int receiverCount,
                                     bool baselineAvailable) {
    int score = 35 + samplesPerChannel / 2;
    if (receiverCount >= 2) score += 15;
    if (baselineAvailable) score += 5;
    return clampPercent(score);
}

inline uint8_t nextEventRun(uint8_t currentRun, uint8_t level,
                            uint8_t threshold, uint8_t hysteresis) {
    if (level >= threshold) return currentRun == 255 ? 255 : currentRun + 1;
    const uint8_t release = threshold > hysteresis ? threshold - hysteresis : 0;
    return level <= release ? 0 : currentRun;
}

}  // namespace AnalyzerMath
