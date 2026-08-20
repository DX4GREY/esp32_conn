#include "core/RfEnvironmentState.h"
#include "core/RfEnvironmentMath.h"

RfEnvironmentState rfEnvironmentState;

void RfEnvironmentState::resetRuntime() {
    memset(channels, 0, sizeof(channels));
    for (int i = 0; i < TOTAL_CHANNELS; ++i) channels[i].minimum = 100;
    memset(history, 0, sizeof(history)); historyHead = historyCount = 0;
    memset(events, 0, sizeof(events)); eventHead = eventCount = 0;
    completedCycles = samplesPerSecond = lastCycleUs = maxCycleUs = 0;
}
uint8_t RfEnvironmentState::averageOccupancy() const {
    uint32_t sum = 0; uint16_t count = 0;
    for (uint8_t ch = config.minChannel; ch <= config.maxChannel; ++ch) { sum += channels[ch].movingAverage; ++count; }
    return count ? sum / count : 0;
}
uint8_t RfEnvironmentState::overallScore() const {
    uint32_t sum = 0; uint16_t count = 0;
    for (uint8_t ch = config.minChannel; ch <= config.maxChannel; ++ch) { sum += channels[ch].score; ++count; }
    return count ? sum / count : 0;
}
const char* RfEnvironmentState::scoreLabel(uint8_t score) const {
    static const char* labels[] = {"CLEAR", "LIGHT", "MODERATE", "BUSY", "SATURATED"};
    return labels[RfEnvironmentMath::classifyScore(score)];
}
void RfEnvironmentState::topChannels(uint8_t* out, uint8_t count) const {
    if (out == nullptr || count == 0) return;
    const uint8_t available = static_cast<uint8_t>(config.maxChannel - config.minChannel + 1U);
    const uint8_t rankedCount = min(count, available);
    for (uint8_t i = 0; i < rankedCount; ++i) out[i] = config.minChannel + i;
    for (uint16_t ch = config.minChannel; ch <= config.maxChannel; ++ch) {
        bool alreadyRanked = false;
        for (uint8_t i = 0; i < rankedCount; ++i) {
            if (out[i] == ch) { alreadyRanked = true; break; }
        }
        if (alreadyRanked) continue;
        for (uint8_t i = 0; i < rankedCount; ++i) {
            if (channels[ch].movingAverage > channels[out[i]].movingAverage) {
                for (uint8_t j = rankedCount - 1; j > i; --j) out[j] = out[j - 1];
                out[i] = static_cast<uint8_t>(ch);
                break;
            }
        }
    }
    for (uint8_t i = rankedCount; i < count; ++i) out[i] = config.minChannel;
}
void RfEnvironmentState::captureSnapshot(RfEnvironmentSnapshot& target) {
    target.valid=true; target.capturedMs=millis(); target.average=averageOccupancy(); target.score=overallScore();
    target.peak=0; target.peakChannel=config.minChannel; target.burstCount=0;
    for (int ch=0;ch<TOTAL_CHANNELS;ch++) { target.channelOccupancy[ch]=channels[ch].movingAverage; target.burstCount+=channels[ch].burstCount;
        if (channels[ch].peak>target.peak) { target.peak=channels[ch].peak; target.peakChannel=ch; }
    }
}
